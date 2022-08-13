/*
 * HdtnModel.cc
 *
 *  Created on: Aug 13, 2022
 *      Author: timothy recker
 *      email: tjr@berkeley.edu
 */

#include "src/hdtn/HdtnModel.h"

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
		throw;
	}

	this->connected = true;
}

void HdtnModel::disconnect()
{
	this->sock->disconnect(this->path);
	delete this->sock;
	delete this->ctx;
	this->connected = false;
}
