#pragma once
#include "TSPrimitives.h"

class TSQueryConstructor : public ISimpleModule
{
public:
	TSQueryConstructor();
	~TSQueryConstructor();

	virtual bool InitParameters(const std::initializer_list<float> &params) override;

	bool ConstructQueryFromDoc(const TSDocumentPtr doc, TSQuery &query) const;
	bool QueryConstructionProcess(const std::string &doc_id, TSQuery &query) const;
	bool QueryExtensionProcess(const TSQuery &query, TSQuery &extended_query) const;

private:
	void CopyFromIndexFirstNItems(const TSIndex &from_index, int n, TSIndex &to_index) const;
	void FillCollectionStatistic(TSIndexConstPtr p_index, int num_of_element, TSIndex &statistic_index) const;
	void CombineStatistics(TSIndex &statistic_index, int query_size) const; 

private:
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

