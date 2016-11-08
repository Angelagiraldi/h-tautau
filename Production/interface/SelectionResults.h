/*! Definition of selection results container used in HH->bbTauTau analysis.
This file is part of https://github.com/hh-italian-group/h-tautau. */

#pragma once

#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "AnalysisTools/Core/include/AnalyzerData.h"
#include "AnalysisTools/Core/include/CutTools.h"
#include "h-tautau/Analysis/include/AnalysisTypes.h"
#include "h-tautau/Analysis/include/Candidate.h"
#include "h-tautau/Analysis/include/KinFitInterface.h"
#include "SVfitInterface.h"

#define SELECTION_ENTRY(name) \
    ANA_DATA_ENTRY(cuts::ObjectSelector, name) \
    /**/

namespace analysis {
class SelectionManager {
public:
    SelectionManager(root_ext::AnalyzerData& _anaData, const std::string& _selection_label, double _weight)
        : anaData(&_anaData), selection_label(_selection_label), weight(_weight) {}

    template<typename ValueType>
    ValueType FillHistogram(ValueType value, const std::string& histogram_name)
    {
        auto& histogram = anaData->Get(static_cast<TH1D*>(nullptr), histogram_name, selection_label);
        return cuts::fill_histogram(value, histogram, weight);
    }

private:
    root_ext::AnalyzerData* anaData;
    std::string selection_label;
    double weight;
};

struct SelectionResultsBase {
    static constexpr size_t NumberOfLegs = 2;
    using TauCandidate = LeptonCandidate<pat::Tau>;
    using JetCandidate = Candidate<pat::Jet>;
    using JetCandidateVector = std::vector<JetCandidate>;
    using Vertex = reco::Vertex;

    edm::EventID eventId;
    EventEnergyScale energyScale;

    bool Zveto, electronVeto, muonVeto, triggerMatch;
    sv_fit::FitResults svfitResult;
    std::map<size_t, kin_fit::FitResults> kinfitResults;
    JetCandidateVector jets;
    const Vertex* primaryVertex;

    SelectionResultsBase(const edm::EventID& _eventId, EventEnergyScale _energyScale) :
        eventId(_eventId), energyScale(_energyScale) {}

    virtual ~SelectionResultsBase() {}
    virtual const LorentzVector& GetHiggsMomentum() const = 0;

    bool HaveSameJets(const SelectionResultsBase& other) const
    {
        static const std::set<EventEnergyScale> jetEnergyScales =
            { EventEnergyScale::JetUp, EventEnergyScale::JetDown };

        if(eventId != other.eventId) return false;
        if(energyScale != other.energyScale
            && (jetEnergyScales.count(energyScale) || jetEnergyScales.count(other.energyScale))) return false;
        if(jets.size() != other.jets.size()) return false;

        for(size_t n = 0; n < jets.size(); ++n) {
            if(&(*jets.at(n)) != &(*other.jets.at(n)))
                return false;
        }
        return true;
    }
};

template<typename _FirstLeg, typename _SecondLeg>
struct SelectionResults : SelectionResultsBase {
    using FirstLeg = _FirstLeg;
    using SecondLeg = _SecondLeg;
    using HiggsCandidate = CompositCandidate<FirstLeg, SecondLeg>;
    using HiggsCandidatePtr = std::shared_ptr<HiggsCandidate>;

    HiggsCandidatePtr higgs;

    using SelectionResultsBase::SelectionResultsBase;

    void SetHiggsCandidate(const HiggsCandidate& h) { higgs = HiggsCandidatePtr(new HiggsCandidate(h)); }
    virtual const LorentzVector& GetHiggsMomentum() const override { return higgs->GetMomentum(); }

    bool HaveSameFirstLegOrigin(const SelectionResults<FirstLeg, SecondLeg>& other) const
    {
        if(eventId != other.eventId) return false;
        return &(*higgs->GetFirstDaughter()) == &(*other.higgs->GetFirstDaughter());
    }

    bool HaveSameSecondLegOrigin(const SelectionResults<FirstLeg, SecondLeg>& other) const
    {
        if(eventId != other.eventId) return false;
        return &(*higgs->GetSecondDaughter()) == &(*other.higgs->GetSecondDaughter());
    }
};

} // namespace analysis
