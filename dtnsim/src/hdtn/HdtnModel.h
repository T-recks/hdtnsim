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
#include <map>
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
//  using HdtnModel::HdtnModel;
  RouterListener(string host, int port, int eid, int nodeNum);
  bool check();
  void connect();
  void disconnect();
  int getNextHop(int dest);
//  int getFinalDest();
  void clear();
  bool clear(int dest);
//  vector<int> getHops();
private:
  map<int, int> nextHop;
//  int finalDest;
//  vector<uint64_t> hops;
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
