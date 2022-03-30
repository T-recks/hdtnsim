/*
 * ContactHistory.cc
 *
 *  Created on: Nov 16, 2021
 *      Author: simon
 */

#include "ContactHistory.h"

ContactHistory::ContactHistory()
{


}

ContactHistory::~ContactHistory()
{
	// TODO Auto-generated destructor stub
}

vector<tuple<Contact,Contact>> ContactHistory::getContactList()
{
	return this->contactList_;
}

map<int,int> ContactHistory::getSenderStartPositions()
{
	return this->sourceStartPositions_;
}

Contact* ContactHistory::getContact(int position, int tuplePosition)
{
	if (tuplePosition == 0){
		return &get<0>(this->contactList_.at(position));
	} else {
		return &get<1>(this->contactList_.at(position));
	}

}

Contact* ContactHistory::getReliableContact(int position)
{
	//the contact at position 0 is considered to be more trustworthy
	if (this->getContact(position, 0)->getId() == -1) {
		return this->getContact(position, 1);
	} else {
		return this->getContact(position, 0);
	}
}

void ContactHistory::addContact(Contact* sourceContact, Contact* destinationContact)
{
	Contact* contact;


	//take source contact with higher priority
	if (sourceContact == NULL) {
		contact = destinationContact;
	} else {
		contact = sourceContact;
	}
	int sourceEid = (*contact).getSourceEid();
	int destinationEid = (*contact).getDestinationEid();
	size_t i = 0;
	bool done = false; //if this is true, the list is not traversed any longer

	if (this->sourceStartPositions_.count(sourceEid) != 0) { //this is not the first time you see this source

		i = this->sourceStartPositions_.at(sourceEid);

		//if the destination/start is smaller the the one(s) of the first contact in the list, the new contact will be the new first.
		if (destinationEid < (*this->getReliableContact(i)).getDestinationEid() ||
				(destinationEid == (*this->getReliableContact(i)).getDestinationEid() && (*contact).getStart() < (*this->getReliableContact(i)).getStart())) {
			done = true;
		}

	} else { //source was not in the list so far

		if (!this->sourceStartPositions_.empty() && (*this->getReliableContact(0)).getSourceEid() < sourceEid) { //new sender not at the beginning

			int difference = sourceEid;
			for (auto it = this->sourceStartPositions_.begin(); it != this->sourceStartPositions_.end(); ++it) { //go through the start position to find the closest position

				if (sourceEid - it->first < difference) {
				    i = it->second;
				    difference = sourceEid - it->first;
				}

			}


		}

		//go through list until position for source is found
		while (i < this->contactList_.size() && (*this->getReliableContact(i)).getSourceEid() < sourceEid) {
				i = i + 1;
		}

		//insert source into list.
		this->sourceStartPositions_.insert({sourceEid, i});
		done = true;
	}


	//go through list until the position for destination was found
	while (!done && i < this->contactList_.size() && (*this->getReliableContact(i)).getDestinationEid() < destinationEid
			&& (*this->getReliableContact(i)).getSourceEid() == sourceEid) {
		i = i + 1;
	}

	//check whether list is ended or its the first destination entry
    if (!(i < this->contactList_.size()) || !((*this->getReliableContact(i)).getSourceEid() == sourceEid)) {
		done = true;
	}

    //find the correct start time position
	while (!done && i < this->contactList_.size() && (*this->getReliableContact(i)).getStart() < (*destinationContact).getStart()
			&& (*this->getReliableContact(i)).getSourceEid() == sourceEid && (*this->getReliableContact(i)).getDestinationEid() == destinationEid) {
		i = i + 1;
	}

	if (i < this->contactList_.size() && !this->isNewContact(i, *contact)) {
		//duplicate was found
		return ;
	}

	tuple<Contact, Contact> new_tuple = make_tuple(*contact, *contact); //dummy value, never used.

	Contact dummy = Contact(-1,0,0,0,0,0,0,0); //dummy contact

	Contact delayed = Contact(destinationContact->getId(), destinationContact->getStart(), destinationContact->getEnd() + (rand() % 2), destinationContact->
			getSourceEid(), destinationContact->getDestinationEid(), destinationContact->getDataRate(), destinationContact->getConfidence(),
					destinationContact->getRange());

	//create a tuple according to availability of contact information
	if (sourceContact == NULL) {
		new_tuple = make_tuple(dummy, delayed);
	} else {
		new_tuple = make_tuple(*sourceContact, delayed);
	}

	//insert contact into history
	auto it = this->contactList_.begin();
    this->contactList_.insert(it + i, new_tuple);

	//update start positions
    for (auto it = this->sourceStartPositions_.begin(); it != this->sourceStartPositions_.end(); ++it) {
		if (it->first > sourceEid) {
			it->second = it->second + 1;
		}

	}

    bool newlyUpdated = true;

    for (size_t i = 0; i < this->updatedContacts_.size(); i++) {
    	if (get<0>(this->updatedContacts_.at(i)) == contact->getSourceEid() && get<1>(this->updatedContacts_.at(i)) == contact->getDestinationEid()) {
    		newlyUpdated = false;
    	}
    }

    if (newlyUpdated) {
    	this->updatedContacts_.push_back(make_tuple(contact->getSourceEid(), contact->getDestinationEid()));
    }

}

void ContactHistory::combineContactHistories(ContactHistory* otherHistory)
{
	vector<tuple<Contact,Contact>> otherHistoryList = (*otherHistory).getContactList();

	//add every contact entry from one history to the other history
	for (long unsigned int i = 0; i < otherHistoryList.size(); i++) {
		this->addContact((*otherHistory).getReliableContact(i), (*otherHistory).getReliableContact(i));
	}


}

Contact ContactHistory::predictAndAddContact(double currentTime, int sourceEid, int destinationEid, ContactPlan *contactPlan)
{
	if (contactPlan != NULL) {
		if ((*contactPlan).hasPredictedContact(sourceEid, destinationEid) || (*contactPlan).hasDiscoveredContact(sourceEid, destinationEid)) {
			return Contact(-1,0,0,0,0,0,0,0);
		}
	}

	vector<Contact*> contacts = this->getAllContactsForSourceDestination(sourceEid, destinationEid);

	if (contacts.size() == 0) {
		return Contact(-1,0,0,0,0,0,0,0);
	}

	Contact* firstContact = contacts.at(0);

	//only predict as far into the future as long as we have seen the contact happening
	double horizon = currentTime + (currentTime - (*firstContact).getStart());

	vector<double> activeIntervals;

	vector<double> inactiveIntervals;

	vector<double> dataRates;

	activeIntervals.push_back((*firstContact).getDuration());

	dataRates.push_back((*firstContact).getDataRate());

	int lastStop = (*firstContact).getEnd();

	//compute durations for active/inactive intervals
	for (size_t i = 1; i < contacts.size(); i++) {
		Contact* contact = contacts.at(i);

		activeIntervals.push_back((*contact).getDuration());

		dataRates.push_back((*contact).getDataRate());

		inactiveIntervals.push_back((*contact).getStart() - lastStop);

		lastStop = (*contact).getEnd();

	}

	//compute means and standard deviations.
	double meanActive = this->calculateMean(activeIntervals);
	double meanInactive = this->calculateMean(inactiveIntervals);
	double standardDeviationActive = this->calculateStandardDeviation(activeIntervals, meanActive);
	double standardDeviationInactive = this->calculateStandardDeviation(inactiveIntervals, meanInactive);

	double meanDataRate = this->calculateMean(dataRates);

	double baseConfidence;

	if (standardDeviationActive < meanActive && standardDeviationInactive < meanInactive) {
		baseConfidence = 0.2;
	} else {
		baseConfidence = 0.05;
	}

	double netConfidence = this->computeNetConfidence(baseConfidence, contacts.size());

	//dummy contact
	Contact newContact(0, currentTime, horizon, sourceEid, destinationEid, meanDataRate, netConfidence, 0);

	//add predicted contact + range directly into the contact plan
	if (contactPlan != NULL) {
		int id = (*contactPlan).addPredictedContact(currentTime, horizon, sourceEid, destinationEid, meanDataRate, netConfidence);
		(*contactPlan).addRange(currentTime, horizon, sourceEid, destinationEid, 0, netConfidence);
		(*contactPlan).getContactById(id)->setRange(0);
	}

	return newContact;
}

vector<Contact*> ContactHistory::getAllContactsForSourceDestination(int sourceEid, int destinationEid)
{
	vector<Contact*> contacts;

	size_t i = this->sourceStartPositions_.at(sourceEid);

	while ( i < this->contactList_.size() && (*this->getReliableContact(i)).getDestinationEid() < destinationEid &&
			(*this->getReliableContact(i)).getSourceEid() == sourceEid) { //go through the source entries until the destination was found!
		i = i+1;
	}

	while ( i < this->contactList_.size() && this->getReliableContact(i)->getDestinationEid() == destinationEid &&
			(*this->getReliableContact(i)).getSourceEid() == sourceEid) { //go through all destination entries!
		contacts.push_back(this->getReliableContact(i));
		i = i+1;
	}

	return contacts;
}

bool ContactHistory::isNewContact(int position, Contact newContact)
{
	Contact contactAtPos = (*this->getReliableContact(position));

	//is every information is equal, contact is considered to be a duplicate
	if (newContact.getSourceEid() == contactAtPos.getSourceEid() && newContact.getDestinationEid() == contactAtPos.getDestinationEid()
			&& newContact.getStart() == contactAtPos.getStart()) {
		return false;
	} else {
		return true;
	}
}


double ContactHistory::calculateMean(vector<double> intervals)
{
	double sum = 0;

	for (long unsigned int i=0; i<intervals.size(); i++) {
		sum = sum + intervals.at(i);
	}

	return (double)sum / (double)intervals.size() ;
}

double ContactHistory::calculateStandardDeviation(vector<double> intervals, double mean)
{
	double sum = 0;

	for (long unsigned int i = 0; i < intervals.size(); i++) {
		sum = sum + (intervals.at(i) - mean) * (intervals.at(i) - mean);
	}

	return sqrt(sum / intervals.size());

}

double ContactHistory::computeNetConfidence(double baseConfidence, int numberOfOccurences)
{
	double product = 1.0;

	for (int i = 0; i < numberOfOccurences; i++) {
		product = product * ((double)1.0 - baseConfidence);
	}

	return (double)1.0 - product;


}

vector<Contact> ContactHistory::predictAndAddAllContacts(double currentTime, ContactPlan *contactPlan)
{
	vector<Contact> contacts;
	vector<Contact> newContacts;

	for (size_t i = 0; i < this->updatedContacts_.size(); i++) {
		Contact contact = this->predictAndAddContact(currentTime, get<0>(this->updatedContacts_.at(i)),
				get<1>(this->updatedContacts_.at(i)), contactPlan);

		contacts.push_back(contact);
	}
	this->updatedContacts_.clear();
	return contacts;
	//go through each possible source node and predict their future contacts!
	for (auto it = this->sourceStartPositions_.begin(); it != this->sourceStartPositions_.end(); ++it) {
		newContacts = this->predictAndAddAllContactsForSource(it->first, currentTime, contactPlan);
		contacts.insert(contacts.end(), newContacts.begin(), newContacts.end());
	}

	return contacts;
}

vector<Contact> ContactHistory::predictAndAddAllContactsForSource(int sourceEid, double currentTime, ContactPlan *contactPlan)
{
	size_t i = this->sourceStartPositions_.at(sourceEid);

	vector<Contact> contacts;

	//go through each possible destination!
	while (i < this->contactList_.size() && (*this->getReliableContact(i)).getSourceEid() == sourceEid) { //don't move into another source!
		int currDestinationEid = (*this->getReliableContact(i)).getDestinationEid();

		Contact contact = this->predictAndAddContact(currentTime, sourceEid, currDestinationEid, contactPlan);

		contacts.push_back(contact);

		i = i + 1;

		while (i < this->contactList_.size() &&(*this->getReliableContact(i)).getDestinationEid() == currDestinationEid
				&& (*this->getReliableContact(i)).getSourceEid() == sourceEid) { //go to the next destination!
			i = i + 1;
		}
	}

	return contacts;
}

