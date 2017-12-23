#pragma once
#include "TSPrimitives.h"

class TSQueryConstructor
{
public:
	TSQueryConstructor();
	~TSQueryConstructor();

	bool InitParameters(const std::initializer_list<float> &params);
	inline bool InitDataExtractor(const class TSDataExtractor *data_extractor) {
		m_pDataExtractor = data_extractor; 
		return true;
	}

	bool ConstructQueryFromDoc(const TSDocument &doc, TSQuery &query) const;
	bool QueryConstructionProcess(const std::string &doc_id, TSQuery &query) const;
	bool QueryExtensionProcess(const TSQuery &query, TSQuery &extended_query) const;

private:
	void CopyFromIndexFirstNItems(const TSIndex &from_index, int n, TSIndex &to_index) const;
	void FillCollectionStatistic(TSIndexConstPtr p_index, int num_of_element, TSIndex &statistic_index) const;
	void CombineStatistics(TSIndex &statistic_index, int query_size) const; 

private:

	const TSDataExtractor *m_pDataExtractor;
	int m_iLemmsFor1L;
	int m_iTermsFor1L;
	int m_iDocCountForQEProcess;
	float m_fSoftOrForQEProcess;
	float m_fMinDocRankForQEProcess;
	int m_iTopLemmsForQEProcess;
	int m_iTopTerminsForQEProcess;
	int m_iResultLemmaSize;
	int m_iResultTerminsSize;
};

