#pragma once

#include "NSCLC/SP_QSP_NSCLC/abm/compartment/Tumor.h"
#include "NSCLC/SP_QSP_NSCLC/abm/compartment/LymphCentral.h"

#include "FileOutputHub.h"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
//! Simulation central control
/*! Host all compartments (currently only one: tumor).
	Handle interaction between compartments; process global statistics output.
*/
namespace NSCLC = SP_QSP_IO::SP_QSP_NSCLC;
class NSCLC_Core
{
public:

public:
	NSCLC_Core();
	virtual ~NSCLC_Core();
	//! setup qsp module
	bool setup_qsp(QSP_IO::Param& p);
	//! Initialize all compartments: random population 
	void initializeSimulation(void);
	//! Proceed on slice in time for all compartments
	virtual void timeSlice(const long slice);
	//! Write ABM stats header to stats file
	void write_stats_header(void) const;
	//! Write ABM stats of current time slice to stats file
	void write_stats_slice(unsigned long slice) const;
	//! write QSP content to file
	void write_QSP(unsigned long slice, bool header)const;
	//! Write grid snapshots of current time slice to file 
	virtual void writeGrids(unsigned long slice, unsigned int option);
	//! Print brief summary of current slice
	virtual void briefStats(unsigned long slice);

private:
	friend class boost::serialization::access;
	//! boost serialization 
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/);

	//! tumor compartment instance
	std::vector<NSCLC::Tumor*> _ROI_core;
	std::vector<NSCLC::Tumor*> _ROI_margin;
	SP_QSP_IO::SP_QSP_NSCLC::LymphCentral _lymph;
};

template<class Archive>
inline void NSCLC_Core::serialize(Archive & ar, const unsigned int version){
	ar & BOOST_SERIALIZATION_NVP(_ROI_core);
	ar & BOOST_SERIALIZATION_NVP(_ROI_margin);
	ar & BOOST_SERIALIZATION_NVP(_lymph);
	SP_QSP_IO::CellAgent::classSerialize(ar, version);
}