#pragma once

#include <vector>						// STL vector<>


// avoids the need to do any multi threading synchronizations
class CPolybumpProgress
{
public:

	enum EPolybumpProgressState
	{
		EPPS_NotStarted,				//
		EPPS_InProgress,				//
		EPPS_DoneP1,						// pass 1 finished but AA missing
		EPPS_DoneFinal					// completely finished
	};

	// constructor
	CPolybumpProgress();

	// set count and clear
	void Init( const uint32 dwIdCount );

	void DeInit();

	void Clear();

	bool IsValid() { return !m_ProgressBlockmap.empty(); }

	// Arguments:
	//   rDest - needs to be of same size
	// Returns:
	//   0xffffffff if there was no change required, otherwise dwId which can be used with GetState()
	uint32 PropagateTo( CPolybumpProgress &rDest ) const;

	// Arguments:
	//   dwId - [0..m_ProgressBlockmap.size()]
	EPolybumpProgressState GetState( const uint32 dwId ) const;

	void SetState( const uint32 dwId, const EPolybumpProgressState state );

	uint32 GetIdCount() const { return (uint32)m_ProgressBlockmap.size(); }

private: // ---------------------------------------------------------------------------------------

	std::vector<EPolybumpProgressState>				m_ProgressBlockmap;						// [dwId] dwId=0..m_ProgressBlockmap.size()-1, values see return of GetState()
	uint32																		m_NextIdToCheckIfPropagated;	//
};
