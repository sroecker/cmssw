// -*- C++ -*-
//
// Package:     Services
// Class  :     Timing
// 
// Implementation:
//
// Original Author:  Jim Kowalkowski
// $Id: Timing.cc,v 1.5 2006/07/31 21:21:56 evansde Exp $
//

#include "FWCore/Services/interface/Timing.h"
#include "FWCore/MessageLogger/interface/JobReport.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "DataFormats/Common/interface/ModuleDescription.h"
#include "DataFormats/Common/interface/EventID.h"
#include "DataFormats/Common/interface/Timestamp.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/Exception.h"

#include <iostream>
#include <sys/time.h>

using namespace std;

namespace edm {
  namespace service {

    static double getTime()
    {
      struct timeval t;
      if(gettimeofday(&t,0)<0)
	throw cms::Exception("SysCallFailed","Failed call to gettimeofday");

      return (double)t.tv_sec + (double(t.tv_usec) * 1E-6);
    }

    Timing::Timing(const ParameterSet& iPS, ActivityRegistry&iRegistry):
      summary_only_(iPS.getUntrackedParameter<bool>("summaryOnly",false)),
      report_summary_(iPS.getUntrackedParameter<bool>("useJobReport",false)),
      max_event_time_(0.),
      min_event_time_(0.),
      total_event_count_(0)
     
    {
      iRegistry.watchPostBeginJob(this,&Timing::postBeginJob);
      iRegistry.watchPostEndJob(this,&Timing::postEndJob);

      iRegistry.watchPreProcessEvent(this,&Timing::preEventProcessing);
      iRegistry.watchPostProcessEvent(this,&Timing::postEventProcessing);

      iRegistry.watchPreModule(this,&Timing::preModule);
      iRegistry.watchPostModule(this,&Timing::postModule);
    }


    Timing::~Timing()
    {
    }

    void Timing::postBeginJob()
    {
      //edm::LogInfo("TimeReport")
      if (not summary_only_) {
        edm::LogSystem("TimeReport")
	<< "TimeReport> Report activated" << "\n"
	<< "TimeReport> Report columns headings for events: "
	<< "eventnum runnum timetaken\n"
	<< "TimeReport> Report columns headings for modules: "
	<< "eventnum runnum modulelabel modulename timetaken";
      }
      curr_job_ = getTime();
    }

    void Timing::postEndJob()
    {
      double t = getTime() - curr_job_;
      double average_event_t = t / total_event_count_;
      //edm::LogInfo("TimeReport")
      edm::LogSystem("TimeReport")
	<< "TimeReport> Time report complete in "
	<< t << " seconds"
	<< "\n"
        << " Time Summary: \n" 
        << " Min: " << min_event_time_ << "\n"
        << " Max: " << max_event_time_ << "\n"
        << " Avg: " << average_event_t << "\n";
      if (report_summary_){
	Service<JobReport> reportSvc;
	std::map<std::string, double> reportData;

	reportData.insert(std::make_pair("MinEventTime", min_event_time_));
	reportData.insert(std::make_pair("MaxEventTime", max_event_time_));
	reportData.insert(std::make_pair("AvgEventTime", average_event_t));
	reportData.insert(std::make_pair("TotalTime", t));
	reportSvc->reportTimingInfo(reportData);
      }

    }

    void Timing::preEventProcessing(const edm::EventID& iID,
				    const edm::Timestamp& iTime)
    {
      curr_event_ = iID;
      curr_event_time_ = getTime();
      

    }
    void Timing::postEventProcessing(const Event& e, const EventSetup&)
    {
      double t = getTime() - curr_event_time_;
      if (not summary_only_) {
        edm::LogSystem("TimeEvent")
	<< "TimeEvent> "
	<< curr_event_.event() << " "
	<< curr_event_.run() << " "
	<< t;
      }
      if (total_event_count_ == 0) {
	max_event_time_ = t;
        min_event_time_ = t;
      }
      
      if (t > max_event_time_) max_event_time_ = t;
      if (t < min_event_time_) min_event_time_ = t;
      total_event_count_ = total_event_count_ + 1;
    }

    void Timing::preModule(const ModuleDescription&)
    {
      curr_module_time_ = getTime();
    }

    void Timing::postModule(const ModuleDescription& desc)
    {
      double t = getTime() - curr_module_time_;
      //edm::LogInfo("TimeModule")
      if (not summary_only_) {
        edm::LogSystem("TimeModule") << "TimeModule> "
	   << curr_event_.event() << " "
	   << curr_event_.run() << " "
	   << desc.moduleLabel_ << " "
	   << desc.moduleName_ << " "
	   << t;
      }
   
      newMeasurementSignal(desc,t);
    }

  }
}
