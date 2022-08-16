/*
 * RouterListener.cc
 *
 *  Created on: Aug 13, 2022
 *      Author: timothy recker
 *      Email: tjr@berkeley.edu
 */

#include "src/hdtn/HdtnModel.h"

RouterListener::RouterListener(string host, int port, int eid, int nodeNum)
: HdtnModel(host, port, eid)
{
	for (int i = 1; i <= nodeNum; ++i) {
//		cout << "node " << eid << " is a destination of " << eid_ << endl;
		nextHop[i] = -1;
	}
}

void RouterListener::connect() {
	HdtnModel::connect(zmq::socket_type::sub);
	this->sock->setsockopt(ZMQ_SUBSCRIBE, "", strlen(""));
//	std::cout << "SET" << std::endl;
	cout << "[listener " << eid_ << "] connected" << endl;
}

void RouterListener::disconnect()
{
	HdtnModel::disconnect();
	cout << "[listener " << eid_ << "] disconnected" << endl;
}

bool RouterListener::check()
{
	bool update = false;
	zmq::pollitem_t items[] = {{this->sock->handle(), 0, ZMQ_POLLIN, 0}};
	int i = 1;

	cout << "[listener " << eid_ << "] polling " << this->path << endl;

	for (int rc = zmq::poll(&items[0], 1, ZMQ_POLL_TIMEOUT);
			rc > 0;
			rc = zmq::poll(&items[0], 1, ZMQ_POLL_TIMEOUT)) {
//	assert (rc >= 0);
//	while (rc > 0) {
		if (items[0].revents & ZMQ_POLLIN) {
			zmq::message_t message;
			while (sock->recv(message, zmq::recv_flags::dontwait)) {
				cerr << "[listener " << eid_ << "] received event " << i << endl;
				++i;
//			zmq::recv_result_t amount = sock->recv(message, zmq::recv_flags::none);
//			int num = amount / sizeof(RouteUpdateHdr);
//			int s = sizeof(RouteUpdateHdr);
//			cout << "[Listener " << eid_ << "] hdr received: " << num << endl;
//			if (!amount) {
//				cerr << "unable to receive message from router" << endl;
//			}
//				cout << "event" << endl;
			if (message.size() < sizeof(CommonHdr)) {
				cout << "[listener " << eid_ << "] unknown message type" << endl;
//					continue;
			}

			CommonHdr *common = (CommonHdr *)message.data();
			switch (common->type) {
			case HDTN_MSGTYPE_ROUTEUPDATE:
			{
				RouteUpdateHdr * routeUpdateHdr = (RouteUpdateHdr *)message.data();
//				RouteUpdateHdr * update2 = routeUpdateHdr+1;
//				cerr << routeUpdateHdr->finalDestEid.nodeId << endl;
//				cerr << update2->finalDestEid.nodeId << endl;
				cbhe_eid_t nextHopEid = routeUpdateHdr->nextHopEid;
				cbhe_eid_t finalDestEid = routeUpdateHdr->finalDestEid;
				int finalDest = finalDestEid.nodeId;
				nextHop[finalDest] = nextHopEid.nodeId;
//					hops(routeUpdateHdr->route);
//					hops = &(routeUpdateHdr->route);
				cout << "[listener " << eid_ << "] route update: use " << nextHopEid.nodeId << "->" << finalDest << endl;
				update = true;
			}
			}
		}
//			cout << "[listener " << eid_ << "] no more messages" << endl;
//		rc = zmq::poll(&items[0], 1, ZMQ_POLL_TIMEOUT);
	}
	}
	return update;
}

int RouterListener::getNextHop(int dest)
{
	return nextHop[dest];
}

void RouterListener::clear() {
	nextHop.clear();
}

bool RouterListener::clear(int dest) {
	int previous = nextHop[dest];
	nextHop[dest] = -1;
	return previous > 0;
}

//int RouterListener::getFinalDest()
//{
//	return finalDest;
//}

//vector<int> RouterListener::getHops() {
//	vector<int> result;
//	for (int cid : hops) {
//		if (cid != -1) {
//			result.push_back(cid);
//		}
//	}
//	return result;
//}
