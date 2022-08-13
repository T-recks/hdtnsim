/*
 * RouterListener.cc
 *
 *  Created on: Aug 13, 2022
 *      Author: timothy recker
 *      Email: tjr@berkeley.edu
 */

#include "src/hdtn/HdtnModel.h"

void RouterListener::connect() {
	HdtnModel::connect(zmq::socket_type::sub);
	this->sock->setsockopt(ZMQ_SUBSCRIBE, "", strlen(""));
//	std::cout << "SET" << std::endl;
	cout << "[listener] connected" << endl;
}

void RouterListener::disconnect()
{
	HdtnModel::disconnect();
	cout << "[listener] disconnected" << endl;
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

int RouterListener::getNextHop()
{
	return nextHop;
}

int RouterListener::getFinalDest()
{
	return finalDest;
}
