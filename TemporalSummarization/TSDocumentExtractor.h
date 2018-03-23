#pragma once
#include "TSPrimitives.h"

class TSDocumentRepresentation
{
public:
	TSDocumentRepresentation(int id) : m_sDate(""), m_bIsW2V(false), m_iID(id), m_iClusterLabel(id), m_TopKTerms(SDataType::LEMMA) {}
	bool InitDocPtr(TSDocumentConstPtr doc_ptr);
	inline void InitIsW2V(bool is_w2v) { m_bIsW2V = is_w2v; }
	inline void InitClusterLabel(int label) { m_iClusterLabel = label; }
	void AddHead(TSSentenceConstPtr sentence_ptr);
	void AddTail(TSSentenceConstPtr sentence_ptr);
	inline TSDocumentConstPtr GetDocPtr() const { return m_pDoc; }
	inline std::string GetDate() const { return m_sDate; }
	inline int GetHourDate() const { return m_iHourDate; }
	inline int GetID() const { return m_iID; }
	inline int GetClusterLabel() const { return m_iClusterLabel; }
	inline void SetImportance(float value) { m_fImportance = value; }
	inline float GetImportance() const { return m_fImportance; }
	float operator*(const TSDocumentRepresentation &other) const;

	inline std::pair<std::string, float> GetPair() const { return std::make_pair(m_pDoc->GetDocID(), m_fImportance); }
	inline TSIndex &GetTopKTerms() { return m_TopKTerms; }
private:
	std::vector<TSSentenceConstPtr> m_HeadSentences;
	std::vector<TSSentenceConstPtr> m_TailSentences;
	TSIndex m_TopKTerms;
	TSDocumentConstPtr m_pDoc;
	float m_fImportance;
	std::string m_sDate;
	int m_iHourDate;
	int m_iID;
	bool m_bIsW2V;
	int m_iClusterLabel;
};

class TSDocumentsCluster
{
public:
	TSDocumentsCluster(TSDocumentRepresentation *init_doc_repr);
	void MergeClusters(TSDocumentsCluster &other_cluster);
	float Sim(const TSDocumentsCluster &other, const std::vector<std::vector<float>> &sim_matrix) const;
	inline const TSIndex& GetClusterCentroid() const { return m_Centroid; }
	inline TSIndex& TSDocumentsCluster::GetClusterCentroid() { return m_Centroid; }
	inline bool IsActive() const { return m_bIsActive; }
	inline int GetClusterLabel() const { return m_iClusterLabel; }
	inline int GetCentroidHourDate() const { return m_iCentroidHourDate; }
	void SetImportance(float imp);
	inline float GetImportance() const { return m_fImportance; }
	inline const TSDocumentRepresentation* GetCentroidDoc() const { return m_pCentroidDoc; }
	inline TSDocumentRepresentation* GetCentroidDoc() { return m_pCentroidDoc; }

	inline std::list<TSDocumentRepresentation*>::iterator begin() { return m_Docs.begin(); }
	inline std::list<TSDocumentRepresentation*>::iterator end() { return m_Docs.end(); }
	inline std::list<TSDocumentRepresentation*>::const_iterator begin() const { return m_Docs.begin(); }
	inline std::list<TSDocumentRepresentation*>::const_iterator end() const { return m_Docs.end(); }
	inline size_t size() const { return m_Docs.size(); }

private:
	void ComputeCentroid();
	void ComputeCentroidDoc();
	void AddDocDataToClusterInfo(TSDocumentRepresentation* doc);

private:
	std::list<TSDocumentRepresentation*> m_Docs;
	int m_iClusterLabel;
	bool m_bIsActive;

	TSDocumentRepresentation *m_pCentroidDoc;
	std::map<TSIndexItemID, float> m_CentroidInfo;
	TSIndex m_Centroid;
	int m_iCentroidHourDate;

	float m_fImportance;
};

class TSDocumentExtractor : public ISimpleModule
{
public:
	TSDocumentExtractor();
	~TSDocumentExtractor();

	virtual bool InitParameters(const std::initializer_list<float> &params) override;
	bool ConstructTimeLineCollections(const TSQuery &query, TSTimeLineCollections &collections);

private:
	bool SeparateCollectionByTime(TSDocCollection &whole_collection, TSTimeLineCollections &collections) const;
	void ComputeDocsImportance(const TSDocCollection &whole_collection, std::map<std::string, float> &doc_to_importance, std::vector<std::string> &top_docs) const;
	void ConstructSimilarityMatrix(const std::vector<TSDocumentRepresentation> &docs_representations, std::vector<std::vector<float>> &similarity_matrix) const;
	void PowerMethod(const std::vector<std::vector<float>> &similarity_matrix, const std::vector<float> &start_importances, std::vector<float> &importances) const;
	static void ReportGraph(const std::vector<std::vector<float>> &similarity_matrix, const std::vector<float> &importances, const std::vector<TSDocumentsCluster> &clusters);
	void ConstructDocumentsRepresentations(const TSDocCollection &whole_collection, std::vector<TSDocumentRepresentation> &docs_representations) const;
	void ConstructDocRepresentation(const TSDocument &document, TSDocumentRepresentation &doc_representation) const;
	bool IsSentenceHasReferenceToThePast(TSSentenceConstPtr sentence) const;
	void CutDaysWithSmallPublicationsSize(TSTimeLineCollections &collections) const;
	void ClusterizationProcess(std::vector<TSDocumentRepresentation> &docs_representations, std::vector<TSDocumentsCluster> &clusters, std::vector<std::vector<float>> &clusters_sim_matrix) const;
	void ConstructClustersSimilarityMatrix(const std::vector<TSDocumentsCluster> &clusters, const std::vector<TSDocumentRepresentation> &docs_representations, const std::vector<std::vector<float>> &similarity_matrix, std::vector<std::vector<float>> &clusters_similarity_matrix) const;
	void ComputeStartImportanceVector(const std::vector<TSDocumentsCluster> &clusters, int docs_size, std::vector<float> &start_importances) const;
	float CalculateHourSim(float h1, float h2) const;
	void NormalizeImportanceVector(std::vector<float> &importances) const;
	bool PrintTopDocs(const TSDocCollection &whole_collection, const std::map<std::string, float> doc_to_importance, const std::vector<std::string> top_docs) const;
private:
	int m_iDocCount;
	float m_fSoftOr;
	float m_fMinDocRank;
	float m_fMinLinkScore;
	float m_fPowerDFactor;
	float m_fDocumentImportanceBoundary;
	bool m_bDocImportance;
	bool m_bTemporalMode;
	bool m_bIsW2V;
	bool m_bPyramidFeature;
	bool m_bClusterization;
	bool m_bLexRank;
	float m_fClusterizationSimThreshold;
	float m_fMaxHourDiff;
	int m_iTopKValue;

	int m_iDocTailSize;
	int m_iDocHeadSize;

	// for reports only
	static int m_iGraphsCount;
};