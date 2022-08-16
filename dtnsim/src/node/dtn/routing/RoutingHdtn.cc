/*
 * RoutingHdtn.cc
 *
 *  Created on: July 26, 2022
 *      Author: timothy recker
 *      Email: tjr@berkeley.edu
 */

#include "RoutingHdtn.h"
#include "src/hdtn/libcgr.h"
#include <signal.h>
#include <cmath>

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
    rootContact.arrival_time = 0;
//	rootContact.arrival_time = (int)ceil(simTime().dbl());
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
		cout << "[RoutingHdtn] placed bundle->" << bundle->getDestinationEid() << " on outduct" << endl;
	} else {
		// no active contact found. store and enqueue the bundle
		enqueue(bundle, nextHop);
		cout << "[RoutingHdtn] bundle->" << bundle->getDestinationEid() << " enqueued to " << nextHop << endl;
	}
}

void RoutingHdtn::contactStart(Contact *c)
{
	sdr_->transferToContact(c);
}

void RoutingHdtn::contactEnd(Contact *c)
{
	hopsTable.clear();

	if (useHdtnRouter) {
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
	}

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
//	path = "/src/hdtn/template.json";
	ifstream temp(path);
	if (!temp) {
		cerr << "can't open template" << endl;
		throw;
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
		throw;
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
