/*
 * HdtnModel.h
 *
 *  Created on: Aug 13, 2022
 *      Author: timothy recker
 *      Email: tjr@berkeley.edu
 */

#ifndef SRC_HDTN_HDTNMODEL_H_
#define SRC_HDTN_HDTNMODEL_H_

#include <string>
#include <iostream>
#include <zmq.hpp>
#include "src/hdtn/message.h"

using namespace std;

class HdtnModel
{
public:
  HdtnModel(string host, int port, int eid);
  ~HdtnModel();
  void connect(zmq::socket_type socktype);
  void disconnect();
protected:
  int port;
  std::string path;
  zmq::socket_t *sock;
  zmq::context_t *ctx;
  bool connected;
  int eid_;
};

class RouterListener : public HdtnModel
{
public:
  using HdtnModel::HdtnModel;
  bool check();
  void connect();
  void disconnect();
  int getNextHop();
  int getFinalDest();
private:
  int nextHop;
  int finalDest;
};

class SchedulerModel : public HdtnModel
{
public:
  using HdtnModel::HdtnModel;
  bool send(IreleaseStopHdr*);
  bool send(IreleaseStartHdr*);
  void connect();
  void disconnect();
private:
};

#endif /* SRC_HDTN_HDTNMODEL_H_ */
