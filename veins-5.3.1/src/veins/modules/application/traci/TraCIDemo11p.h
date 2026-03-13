//
// Copyright (C) 2006-2011 Christoph Sommer <christoph.sommer@uibk.ac.at>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#pragma once

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include <fstream>

namespace veins {

/**
 * @brief
 * A tutorial demo for TraCI. When the car is stopped for longer than 10 seconds
 * it will send a message out to other cars containing the blocked road id.
 * Receiving cars will then trigger a reroute via TraCI.
 * When channel switching between SCH and CCH is enabled on the MAC, the message is
 * instead send out on a service channel following a Service Advertisement
 * on the CCH.
 *
 * @author Christoph Sommer : initial DemoApp
 * @author David Eckhoff : rewriting, moving functionality to DemoBaseApplLayer, adding WSA
 *
 */

class VEINS_API TraCIDemo11p : public DemoBaseApplLayer {
public:
    void initialize(int stage) override;

protected:
    simtime_t lastDroveAt;
    bool sentMessage;
    int currentSubscribedServiceId;

    // --- NEW ADDITIONS FOR RESEARCH ---
    bool useTimeRotation; // True or False
    double timeThreshold; // Rotation Trigger Value (Time-based)
    bool useDistanceRotation; // False or True
    double distanceThreshold; // Rotation Trigger Value (Distance-based)

    simtime_t lastRotationTime; // Used for Time-based rotation
    veins::Coord lastRotationPosCoord; // The XY coordinate tracker
    double accumulatedDistanceForRotation; // The distance counter (Distance-based rotation)
    bool isSilent; // Silent Period flag
    double silentPeriod; // Silent Period duration

    static int nextRealID;       // Counter for unique Birth IDs
    static int nextPseudonym;    // Counter for unique Pseudonyms

    int myStaticRealID;          // This car's permanent "Ground Truth"
    int myCurrentPseudonym;      // This car's current "Over-the-Air" ID
    // ----------------------------------

protected:
    void onWSM(BaseFrame1609_4* wsm) override;
    void onWSA(DemoServiceAdvertisment* wsa) override;

    void handleSelfMsg(cMessage* msg) override;
    void handlePositionUpdate(cObject* obj) override;

    // --- NEW ADDITIONS FOR RESEARCH ---
    void changePseudonym();
    // ----------------------------------
};

} // namespace veins
