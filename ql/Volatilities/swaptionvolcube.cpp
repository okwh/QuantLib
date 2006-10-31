/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2006 Ferdinando Ametrano
 Copyright (C) 2006 Katiuscia Manzoni

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/reference/license.html>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/


#include <ql/Volatilities/swaptionvolcube.hpp>

namespace QuantLib {

    SwaptionVolatilityCube::SwaptionVolatilityCube(
        const Handle<SwaptionVolatilityStructure>& atmVolStructure,
        const std::vector<Period>& expiries,
        const std::vector<Period>& lengths,
        const std::vector<Spread>& strikeSpreads,
        const Calendar& calendar,
        const boost::shared_ptr<SwapIndex>& swapIndexBase)
    : SwaptionVolatilityStructure(0, calendar),
      atmVolStructure_(atmVolStructure),
      exerciseDates_(expiries.size()),
      exerciseTimes_(expiries.size()),
      exerciseDatesAsReal_(expiries.size()),
      lengths_(lengths),
      timeLengths_(lengths.size()),
      nStrikes_(strikeSpreads.size()),
      strikeSpreads_(strikeSpreads),
      localStrikes_(nStrikes_),
      localSmile_(nStrikes_),
      swapIndexBase_(swapIndexBase)
    {

        if (!atmVolStructure_.empty())
            unregisterWith(atmVolStructure_);
        atmVolStructure_ = atmVolStructure;
        if (!atmVolStructure_.empty())
            registerWith(atmVolStructure_);
        notifyObservers();


       // register with SwapIndexBase
        if (!swapIndexBase_)
            registerWith(swapIndexBase_);


        nExercise_ = expiries.size();
        exerciseDates_[0] = calendar.advance(referenceDate(),
                                             expiries[0],
                                             Following); //FIXME
        exerciseDatesAsReal_[0] =
            static_cast<Real>(exerciseDates_[0].serialNumber());
        exerciseTimes_[0] = timeFromReference(exerciseDates_[0]);
        QL_REQUIRE(0.0<exerciseTimes_[0],
                   "first exercise time is negative ("
                   << exerciseTimes_[0] << ")");
        for (Size i=1; i<nExercise_; i++) {
            exerciseDates_[i] = calendar.advance(referenceDate(),
                                                 expiries[i],
                                                 Following); //FIXME
            exerciseDatesAsReal_[i] =
                static_cast<Real>(exerciseDates_[i].serialNumber());
            exerciseTimes_[i] = timeFromReference(exerciseDates_[i]);
            QL_REQUIRE(exerciseTimes_[i-1]<exerciseTimes_[i],
                       "non increasing exercise times: time[" << i-1 <<
                       "] = " << exerciseTimes_[i-1] << ", time[" << i <<
                       "] = " << exerciseTimes_[i]);
        }

        exerciseInterpolator_ =
            LinearInterpolation(exerciseTimes_.begin(),
                                exerciseTimes_.end(),
                                exerciseDatesAsReal_.begin());
        exerciseInterpolator_.enableExtrapolation();

        nlengths_ = lengths_.size();
        Date startDate = exerciseDates_[0]; // as good as any
        Date endDate = startDate + lengths_[0];
        timeLengths_[0] = dayCounter().yearFraction(startDate,endDate);
        QL_REQUIRE(0.0<timeLengths_[0],
                   "first time length is negative");
        for (Size i=1; i<nlengths_; i++) {
            Date endDate = startDate + lengths_[i];
            timeLengths_[i] = dayCounter().yearFraction(startDate,endDate);
            QL_REQUIRE(timeLengths_[i-1]<timeLengths_[i],
                       "non increasing time length");
        }

        QL_REQUIRE(nStrikes_>1, "too few strikes (" << nStrikes_ << ")");
        for (Size i=1; i<nStrikes_; i++) {
            QL_REQUIRE(strikeSpreads_[i-1]<strikeSpreads_[i],
                "non increasing strike spreads");
        }
    }

    Rate SwaptionVolatilityCube::atmStrike(const Date& exerciseDate,
                                           const Period& swapTenor) const {

        // FIXME use a familyName-based index factory and
        // use a Period-based SwapIndex constructor
        return SwapIndex(swapIndexBase_->familyName(),
                         swapTenor.units(),
                         swapIndexBase_->settlementDays(),
                         swapIndexBase_->currency(),
                         swapIndexBase_->calendar(), 
                         swapIndexBase_->fixedLegFrequency(),
                         swapIndexBase_->fixedLegConvention(),
                         swapIndexBase_->dayCounter(), 
                         swapIndexBase_->iborIndex()).fixing(exerciseDate);
    }

}
