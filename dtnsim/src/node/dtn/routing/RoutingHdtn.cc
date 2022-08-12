#include "RoutingHdtn.h"
#include "src/hdtn/libcgr.h"
#include <signal.h>
#include <cmath>

#define ZMQ_POLL_TIMEOUT 50

RoutingHdtn::RoutingHdtn(int eid, SdrModel * sdr, ContactPlan * contactPlan, string * path, string * cpJson, bool useHdtnRouter)
: RoutingDeterministic(eid, sdr, contactPlan)
{
	if (this->eid_ > 0) {
		this->hdtnSourceRoot = string(*path);
		this->cpFile = string(*cpJson);
		this->useHdtnRouter = useHdtnRouter;
		if (useHdtnRouter) {
			this->route_fn = &RoutingHdtn::routeHdtn;
			this->listener = new RouterListener("localhost", HDTN_BOUND_ROUTER_PUBSUB_PATH + this->eid_, this->eid_);
			listener->connect();
			this->scheduler = new SchedulerModel("127.0.0.1", HDTN_BOUND_SCHEDULER_PUBSUB_PATH + this->eid_, this->eid_);
			scheduler->connect();
				createRouterConfigFile();
				runHdtn(3, 1);
		} else {
			this->route_fn = &RoutingHdtn::routeLibcgr;
		}
	}
}

RoutingHdtn::~RoutingHdtn()
{
	if (this->eid_ > 0) {
		if (useHdtnRouter) {
//			killHdtn();
			scheduler->disconnect();
			listener->disconnect();
			delete scheduler;
			delete listener;
		}
	}
}

void RoutingHdtn::runHdtn(int destEid, int serviceClass) {
	string hdtnExec = this->hdtnSourceRoot + "/build/module/router/hdtn-router";
	string execString(
		hdtnExec +
		string(" --contact-plan-file=") + this->cpFile +
		string(" --dest-uri-eid=ipn:") + to_string(destEid) + string(".") + to_string(serviceClass) +
		string(" --hdtn-config-file=") + this->configFile +
		string(" &"));

	cout << "[RoutingHdtn] Running command: " << endl << execString << endl;
	system(execString.c_str());
}

void RoutingHdtn::killHdtn() {
	char pidline[16];
	FILE *cmd = popen("pidof hdtn-router", "r");
	fgets(pidline, 16, cmd);
	pid_t pid = strtoul(pidline, NULL, 10);
	kill(pid, SIGTERM);
	pclose(cmd);
}

int RoutingHdtn::routeHdtn(BundlePkt * bundle) {
	// wait to receive message from router
	while(!listener->check());

	return listener->getNextHop();
}

int RoutingHdtn::routeLibcgr(BundlePkt * bundle) {
	string jsonEventFileName = this->hdtnSourceRoot + "/module/router/src/" + this->cpFile;
    vector<cgr::Contact> contactPlan = cgr::cp_load(jsonEventFileName);
    cgr::Contact rootContact = cgr::Contact(this->eid_, this->eid_, 0, cgr::MAX_SIZE, 100, 1.0, 0);
//    rootContact.arrival_time = 0;
	rootContact.arrival_time = (int)ceil(simTime().dbl());
	cgr::Route bestRoute = cgr::dijkstra(&rootContact, bundle->getDestinationEid(), contactPlan);
	return bestRoute.next_node;
}

void RoutingHdtn::routeAndQueueBundle(BundlePkt * bundle, double simTime)
{
	int nextHop;
	int dest = bundle->getDestinationEid();

	map<int, int>::iterator entry = hopsTable.find(dest);
	if (entry == hopsTable.end()) {
		nextHop = (this->*route_fn)(bundle);
		hopsTable[dest] = nextHop;
	} else {
		nextHop = entry->second;
	}

	cout << "[RoutingHdtn " << this->eid_ << "] next hop is " << nextHop << endl;

	// transmit or enqueue
	bool success = attemptTransmission(bundle, nextHop);
	if (success) {
		// an active contact was found so try to transmit on it
		cout << "[RoutingHdtn] placed bundle on outduct to active contact" << endl;
	} else {
		// no active contact found. store and enqueue the bundle
		enqueue(bundle, nextHop);
		cout << "[RoutingHdtn] enqueued bundle to storage" << endl;
	}
}

void RoutingHdtn::contactStart(Contact *c)
{
	sdr_->transferToContact(c);
}

void RoutingHdtn::contactEnd(Contact *c)
{
//	if (useHdtnRouter) {
	//	hopsTable.erase(destTable[c->getId()]);
		hopsTable.clear();
		IreleaseStopHdr msg;
		cbhe_eid_t next;
		next.nodeId = c->getDestinationEid();
		next.serviceId = 1;
		cbhe_eid_t prev;
		prev.nodeId = c->getSourceEid();
		prev.serviceId = 1;
		memset(&msg, 0, sizeof(IreleaseStopHdr));

		msg.base.type = HDTN_MSGTYPE_ILINKDOWN;
		msg.nextHopEid = next;
		msg.prevHopEid = prev;

		msg.contact = (uint64_t)c->getId();
		msg.time = ceil(simTime().dbl());
	//	cout << "[scheduler] time calculated is " << msg.time << endl;

		if (!scheduler->send(&msg)) {
			cerr << "[scheduler] unable to send" << endl;
		} else {
			cout << "[scheduler " << this->eid_ << "] SENT link up/down" << endl;
		}
//	}

	RoutingDeterministic::contactEnd(c);
}

bool RoutingHdtn::attemptTransmission(BundlePkt * bundle, int neighborNodeNbr)
{
	omnetpp::simtime_t time = simTime();
	bundle->setNextHopEid(neighborNodeNbr);
	for (Contact &c : contactPlan_->getContactsBySrcDst(eid_, neighborNodeNbr)) {
		if (c.isActive(time.dbl())) {
			// put bundle directly on active contact
			sdr_->transferToContact(&c, bundle);
			return true;
		}
	}
	return false;
}

void RoutingHdtn::enqueue(BundlePkt * bundle, int neighborNodeNbr)
{
	bundle->setNextHopEid(neighborNodeNbr);
	sdr_->enqueueBundleToNode(bundle, neighborNodeNbr);

}

void RoutingHdtn::createRouterConfigFile()
{
	cout << "[RoutingHdtn] creating configs for node " << this->eid_ << endl;
	char cwd[1024];
	string path;

	getcwd(cwd, sizeof(cwd));
	path = string(cwd) + "/" + "template.json";
	ifstream temp(path);
	if (!temp) {
		cerr << "can't open template" << endl;
	}

	chdir("hdtnFiles");
	getcwd(cwd, sizeof(cwd));
	setenv("HDTN_NODE_LIST_DIR", cwd, 1);

	path = "node" + to_string(this->eid_);
	mkdir(path.c_str(), 0700);
	chdir(path.c_str());

	ofstream file("cfg.json");
	if (!file) {
		cerr << "can't open cfg" << endl;
	}

	file << temp.rdbuf();
	file << "    \"myNodeId\": " << this->eid_ << "," << endl;
	file << "    \"zmqRouterAddress\": \"localhost\"," << endl;
	file << "    \"zmqSchedulerAddress\": \"127.0.0.1\"," << endl;
	file << "    \"zmqBoundRouterPubSubPortPath\": " << HDTN_BOUND_ROUTER_PUBSUB_PATH + this->eid_ << "," << endl;
	file << "    \"zmqBoundSchedulerPubSubPortPath\": " << HDTN_BOUND_SCHEDULER_PUBSUB_PATH + this->eid_ << endl;
	file << "}" << endl;
	temp.close();
	file.close();

	chdir("../../");

	this->configFile = string(cwd) + "/" + path + "/cfg.json";
}

HdtnModel::HdtnModel(string host, int port, int eid)
: port(port)
{
	this->eid_ = eid;
	this->path = string("tcp://") + host + ":" + to_string(port);
}

HdtnModel::~HdtnModel()
{
	if (this->connected) {
		disconnect();
	}
}

void HdtnModel::connect(zmq::socket_type socktype)
{
	try {
		this->ctx = new zmq::context_t();
		this->sock = new zmq::socket_t(*ctx, socktype);
		this->sock->connect(this->path);
	} catch (const zmq::error_t &ex) {
		cerr << "HdtnModel: " << ex.what() << endl;
	}

	this->connected = true;
}

void RouterListener::connect() {
	HdtnModel::connect(zmq::socket_type::sub);
	this->sock->setsockopt(ZMQ_SUBSCRIBE, "", strlen(""));
//	std::cout << "SET" << std::endl;
	cout << "[listener] connected" << endl;
}

void SchedulerModel::connect() {
//	HdtnModel::connect(zmq::socket_type::pub);
	try {
		this->ctx = new zmq::context_t();
		this->sock = new zmq::socket_t(*ctx, zmq::socket_type::pub);
		this->sock->bind(this->path);
		cout << "[scheduler] bound to " << this->path << endl;
		this->connected = true;
	} catch (const zmq::error_t &ex) {
		cerr << "[scheduler] unable to bind: " << ex.what() << endl;
	}
}

void HdtnModel::disconnect()
{
	this->sock->disconnect(this->path);
	delete this->sock;
	delete this->ctx;
	this->connected = false;
}

void RouterListener::disconnect()
{
	HdtnModel::disconnect();
	cout << "[listener] disconnected" << endl;
}

void SchedulerModel::disconnect()
{
	HdtnModel::disconnect();
	cout << "[scheduler] disconnected" << endl;
}

bool RouterListener::check()
{
	zmq::pollitem_t items[] = {{this->sock->handle(), 0, ZMQ_POLLIN, 0}};
	cout << "[listener " << this->eid_ << "] polling at " << this->path << endl;

	int rc = zmq::poll(&items[0], 1, ZMQ_POLL_TIMEOUT);
	assert(rc >= 0);

	if (rc > 0) {
		if (items[0].revents & ZMQ_POLLIN) {
			zmq::message_t message;
			if (!sock->recv(message, zmq::recv_flags::none)) {
				cout << "[listener] unable to receive message" << endl;
				return false;
			}

			if (message.size() < sizeof(CommonHdr)) {
				cout << "[listener] unknown message type" << endl;
				return false;
			}

			CommonHdr *common = (CommonHdr *)message.data();
			switch (common->type) {
				case HDTN_MSGTYPE_ROUTEUPDATE:
				{
					RouteUpdateHdr * routeUpdateHdr = (RouteUpdateHdr *)message.data();
					cbhe_eid_t nextHopEid = routeUpdateHdr->nextHopEid;
					cbhe_eid_t finalDestEid = routeUpdateHdr->finalDestEid;
					nextHop = nextHopEid.nodeId;
					finalDest = finalDestEid.nodeId;
					cout << "[listener " << this->eid_ << "] received route update: use " << nextHop << " to " << finalDest << endl;
					return true;
				}
			}
		}
	}
	return false;
}

bool SchedulerModel::send(IreleaseStopHdr *msg)
{
	return (bool) sock->send(
			zmq::const_buffer(msg, sizeof(IreleaseStopHdr)),
			zmq::send_flags::none);
}

bool SchedulerModel::send(IreleaseStartHdr *msg)
{
	return (bool) sock->send(
			zmq::const_buffer(msg, sizeof(IreleaseStartHdr)),
			zmq::send_flags::none);
}

int RouterListener::getNextHop()
{
	return nextHop;
}

int RouterListener::getFinalDest()
{
	return finalDest;
}









