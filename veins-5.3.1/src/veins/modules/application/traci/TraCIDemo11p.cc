//
// Copyright (C) 2006-2011 Christoph Sommer <christoph.sommer@uibk.ac.at>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "veins/modules/application/traci/TraCIDemo11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <fstream>

using namespace veins;

// Static member initialization
int veins::TraCIDemo11p::nextRealID = 1;
int veins::TraCIDemo11p::nextPseudonym = 1000;

Define_Module(veins::TraCIDemo11p);

void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;

        // Privacy Parameters
        isSilent = false;
        silentPeriod = par("silentPeriod").doubleValue();
        useTimeRotation = par("useTimeRotation").boolValue();
        timeThreshold = par("timeThreshold").doubleValue();
        useDistanceRotation = par("useDistanceRotation").boolValue();
        distanceThreshold = par("distanceThreshold").doubleValue();

        // Identity Initialization
        myStaticRealID = nextRealID++;
        myCurrentPseudonym = nextPseudonym++;
        myId = myCurrentPseudonym;

        // Rotation Tracking
        lastRotationTime = simTime();
        accumulatedDistanceForRotation = 0;
        lastRotationPosCoord = mobility->getPositionAt(simTime());

        // --- CSV FILE INITIALIZATION ---
        static bool fileInitialized = false;
        if (!fileInitialized) {
            std::ofstream outFile;
            outFile.open("adversary_results.csv", std::ios::out | std::ios::trunc);
            if (outFile.is_open()) {
                outFile << "Time,RealID,myCurrentPseudonym,X,Y\n";
                outFile.close();
            }
            fileInitialized = true;
        }

        // --- START 10Hz HEARTBEAT (0.1s) ---
        scheduleAt(simTime() + 0.1, new cMessage("heartbeat"));

        EV << "VEHICLE_INITIALIZED: RealID=" << myStaticRealID << " InitialPseudo=" << myCurrentPseudonym << endl;
    }
}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa)
{
    if (isSilent) return; // Don't respond if in silent period
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Traffic Service");
        }
    }
}

void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
{
    // If the car is in a Silent Period, it shouldn't even "process" incoming data
    // for the sake of the simulation's privacy logic.
    if (isSilent) return;

    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    // Visual feedback in the OMNeT++ GUI (Turn the car green when it hears a message)
    findHost()->getDisplayString().setTagArg("i", 1, "green");

    // Basic Veins routing logic: If the message contains a new route, follow it.
    if (mobility->getRoadId()[0] != ':') {
        traciVehicle->changeRoute(wsm->getDemoData(), 9999);
    }

    // WE REMOVED THE scheduleAt(...) BLOCK HERE.
    // The car will no longer re-broadcast messages it receives.
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
    // 1. Handle Silent Period end
    if (std::string(msg->getName()) == "resumeTalking") {
        isSilent = false;

        // Reset rotation metrics
        lastRotationTime = simTime();
        accumulatedDistanceForRotation = 0;

        delete msg;
        return;
    }

    // 2. The 10Hz Heartbeat
    if (std::string(msg->getName()) == "heartbeat") {
        // Only log if we are NOT silent
        if (!isSilent) {
            // Get position directly from the mobility module
            Coord currentPos = mobility->getPositionAt(simTime());

            std::ofstream outFile;
            outFile.open("adversary_results.csv", std::ios_base::app);
            if (outFile.is_open()) {
                // Time, RealID, PseudoID, X, Y
                outFile << simTime() << "," << myStaticRealID << ","
                        << myCurrentPseudonym << "," << currentPos.x << "," << currentPos.y << "\n";
                outFile.close();
            }

            TraCIDemo11pMessage* bsm = new TraCIDemo11pMessage();
            bsm->setSenderAddress(myCurrentPseudonym);
            bsm->setRecipientAddress(-1); // Broadcast
            sendDown(bsm);
        }

        // Always schedule next heartbeat to keep the loop alive
        scheduleAt(simTime() + 0.1, msg);
        return;
    }

    // 3. Handle other messages (Original Logic)
    if (TraCIDemo11pMessage* wsm = dynamic_cast<TraCIDemo11pMessage*>(msg)) {
        if (!isSilent) sendDown(wsm->dup());

        wsm->setSerial(wsm->getSerial() + 1);
        if (wsm->getSerial() >= 3) {
            delete (wsm);
        } else {
            scheduleAt(simTime() + 1, wsm);
        }
    } else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);
    Coord currentPos = mobility->getPositionAt(simTime());

    // Update accumulated distance for rotation logic
    double d = lastRotationPosCoord.distance(currentPos);
    accumulatedDistanceForRotation += d;
    lastRotationPosCoord = currentPos;

    // Check if identity rotation is triggered by Time or Distance
    if (!isSilent) {
        if (useTimeRotation && (simTime() - lastRotationTime >= timeThreshold)) {
            changePseudonym();
        }
        else if (useDistanceRotation && (accumulatedDistanceForRotation >= distanceThreshold)) {
            changePseudonym();
        }
    }

    // Speed-based logic (original example)
    if (mobility->getSpeed() < 1) {
        if (simTime() - lastDroveAt >= 10 && !sentMessage) {
            findHost()->getDisplayString().setTagArg("i", 1, "red");
            sentMessage = true;
            TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
            populateWSM(wsm);
            wsm->setDemoData(mobility->getRoadId().c_str());
            sendDown(wsm);
        }
    } else {
        lastDroveAt = simTime();
    }
}

void TraCIDemo11p::changePseudonym()
{
    // Rotate ID
    myCurrentPseudonym = nextPseudonym++;
    myId = myCurrentPseudonym;

    // ENTER SILENT PERIOD
    // During this time, handleSelfMsg will stop logging and broadcasting.
    isSilent = true;
    silentPeriod = par("silentPeriod").doubleValue();
    scheduleAt(simTime() + silentPeriod, new cMessage("resumeTalking"));

    EV << "PSEUDONYM_ROTATION: RealID=" << myStaticRealID << " triggered silence for " << silentPeriod << "s" << endl;
}
