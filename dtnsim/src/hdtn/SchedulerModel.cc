/*
 * SchedulerModel.cc
 *
 *  Created on: Aug 13, 2022
 *      Author: timothy recker
 *      Email: tjr@berkeley.edu
 */

#include "src/hdtn/HdtnModel.h"

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
		throw;
	}
}

void SchedulerModel::disconnect()
{
	HdtnModel::disconnect();
	cout << "[scheduler] disconnected" << endl;
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
