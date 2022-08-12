#ifndef SRC_NODE_DTN_ROUTINGHDTN_H_
#define SRC_NODE_DTN_ROUTINGHDTN_H_

#include <zmq.hpp>
#include <src/node/dtn/routing/RoutingDeterministic.h>
#include <src/node/dtn/SdrModel.h>
#include "src/hdtn/message.h"

#define HDTN_ROUTER_ADDRESS "localhost"
#define HDTN_BOUND_SCHEDULER_PUBSUB_PATH 10000
#define HDTN_BOUND_ROUTER_PUBSUB_PATH 11000


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
//  RouterListener(int port);
//  ~RouterListener();
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
//  SchedulerModel(int port);
//  ~SchedulerModel();
  using HdtnModel::HdtnModel;
  bool send(IreleaseStopHdr*);
  bool send(IreleaseStartHdr*);
  void connect();
  void disconnect();
};

class RoutingHdtn : public RoutingDeterministic
{
public:
  RoutingHdtn(int eid, SdrModel *sdr, ContactPlan *contactPlan, string *path, string *cpJson, bool useHdtnRouter);
  virtual ~RoutingHdtn();
  virtual void routeAndQueueBundle(BundlePkt* bundle, double simTime);
  virtual void contactStart(Contact *c);
  virtual void contactEnd(Contact *c);
private:
  map<int, int> hopsTable; // map dest->hop to memoize for simulator performance
  map<int, int> destTable; // map contact->dest so we can check when to reset hopsTable
  RouterListener *listener;
  SchedulerModel *scheduler;
  bool useHdtnRouter;
  std::string hdtnSourceRoot;
  std::string cpFile;
  std::string configFile;
  int (RoutingHdtn::*route_fn)(BundlePkt *bundle);
  virtual void runHdtn(int eid, int service);
  virtual void killHdtn();
  virtual int routeHdtn(BundlePkt *bundle);
  virtual int routeLibcgr(BundlePkt *bundle);
  virtual bool attemptTransmission(BundlePkt *bundle, int neighborNodeNbr);
  virtual void enqueue(BundlePkt *bundle, int neighborNodeNbr);
  virtual void createRouterConfigFile();
};

#endif /* SRC_NODE_DTN_ROUTINGHDTN_H_ */
