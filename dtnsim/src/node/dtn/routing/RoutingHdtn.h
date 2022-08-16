/*
 * RoutingHdtn.h
 *
 *  Created on: July 26, 2022
 *      Author: timothy recker
 *      Email: tjr@berkeley.edu
 */

#ifndef SRC_NODE_DTN_ROUTINGHDTN_H_
#define SRC_NODE_DTN_ROUTINGHDTN_H_

#include <zmq.hpp>
#include <src/node/dtn/routing/RoutingDeterministic.h>
#include <src/node/dtn/SdrModel.h>
#include "src/hdtn/message.h"
#include "src/hdtn/HdtnModel.h"

#define HDTN_ROUTER_ADDRESS "localhost"
#define HDTN_SCHEDULER_ADDRESS "127.0.0.1"
#define HDTN_BOUND_SCHEDULER_PUBSUB_PATH 10000
#define HDTN_BOUND_ROUTER_PUBSUB_PATH 11000

class RoutingHdtn : public RoutingDeterministic
{
public:
  RoutingHdtn(int eid, SdrModel *sdr, ContactPlan *contactPlan, string *path, string *cpJson, bool useHdtnRouter, int num);
  virtual ~RoutingHdtn();
  virtual void routeAndQueueBundle(BundlePkt* bundle, double simTime);
  virtual void contactStart(Contact *c);
  virtual void contactEnd(Contact *c);
private:
  vector<int> destinations;
//  map<int, int> hopsTable; // map dest->hop to memoize for simulator performance
  map<int, vector<int>> destTable; // map contact->dest so we can check when to reset hopsTable
  RouterListener *listener;
  SchedulerModel *scheduler;
  bool useHdtnRouter_;
  int nodeNum_;
  std::string hdtnSourceRoot;
  std::string cpFile;
  std::string configFile;
  int (RoutingHdtn::*route_fn)(BundlePkt *bundle);
  virtual void runHdtn();
  virtual void killHdtn();
  virtual int routeHdtn(BundlePkt *bundle);
  virtual int routeLibcgr(BundlePkt *bundle);
  virtual bool attemptTransmission(BundlePkt *bundle, int neighborNodeNbr);
  virtual void enqueue(BundlePkt *bundle, int neighborNodeNbr);
  virtual void createRouterConfigFile();
};

#endif /* SRC_NODE_DTN_ROUTINGHDTN_H_ */
