/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include <iostream>
#include "ns3/core-module.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/command-line.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/fncs-simulator-impl.h"

/**
 * \file
 * \ingroup simulator
 * Example program demonstrating use of various Schedule functions.
 */

using namespace ns3;

class MyModel
{
public:
  void Start (void);
private:
  void HandleEvent (double eventValue);
};

void
MyModel::Start (void)
{
  Simulator::Schedule (Seconds (10.0),
                       &MyModel::HandleEvent,
                       this, Simulator::Now ().GetSeconds ());
}
void
MyModel::HandleEvent (double value)
{
  std::cout << "Member method received event at "
            << Simulator::Now ().GetSeconds ()
            << "s started at " << value << "s" << std::endl;
}

static void
ExampleFunction (MyModel *model)
{
  std::cout << "ExampleFunction received event at "
            << Simulator::Now ().GetSeconds () << "s" << std::endl;
  model->Start ();
}

static void
RandomFunction (void)
{
  std::cout << "RandomFunction received event at "
            << Simulator::Now ().GetSeconds () << "s" << std::endl;
}

static void
CancelledEvent (void)
{
  std::cout << "I should never be called... " << std::endl;
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Fncs simulation setup
  Ptr<FncsSimulatorImpl> sim = CreateObject<FncsSimulatorImpl> ();
  Simulator::SetImplementation(sim);
  
  //Define jitter parameters to simulate lack of total synchronicity in all objects
  Config::SetDefault ("ns3::FncsApplication::JitterMinNs", DoubleValue (10));
  Config::SetDefault ("ns3::FncsApplication::JitterMaxNs", DoubleValue (100));

  MyModel model;
  Ptr<UniformRandomVariable> v = CreateObject<UniformRandomVariable> ();
  v->SetAttribute ("Min", DoubleValue (10));
  v->SetAttribute ("Max", DoubleValue (20));

  Simulator::Schedule (Seconds (10.0), &ExampleFunction, &model);

  Simulator::Schedule (Seconds (v->GetValue ()), &RandomFunction);

  EventId id = Simulator::Schedule (Seconds (30.0), &CancelledEvent);
  Simulator::Cancel (id);

  // schedule when to end the simulation
  Simulator::Stop (Seconds (100.0));

  Simulator::Run ();

  Simulator::Destroy ();
}