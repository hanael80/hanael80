////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file	TimeValueUpdater.cpp
///
/// @brief	TimeValueUpdater 클래스의 소스 파일
///
/// @date	2015-7-23
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "TimeValueUpdater.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	생성자
////////////////////////////////////////////////////////////////////////////////////////////////////
TimeValueUpdater::Parameter::Parameter()
{
	Reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	이 객체를 처음 상태로 되돌린다.
/// 
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
void TimeValueUpdater::Parameter::Reset()
{
	minValue               = 0;
	curTimeInMillis        = 0;
	updateDuration         = 0;
	effectIdForCurValue    = -1;
	effectIdForMaxValue    = -1;
	effectIdForDuration    = -1;
	effectIdForUpdateValue = -1;
	remainingSeconds       = -1;
	downwardCorrection     = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	값을 갱신한다.
/// 
/// @return	데이터 변동 여부
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TimeValueUpdater::Parameter::Update()
{
	return TimeValueUpdater::Update( *this );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	값을 갱신한다.
/// 
/// @param	parameter	파ㅏ미터
/// 
/// @return	데이터 변동 여부
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TimeValueUpdater::Update( Parameter& parameter )
{
	_CorrectParameter( parameter );

	long long curTimeInMillis = _GetCurTimeInMillis( parameter );

	TimeEffectValueList buffEffects;
	_GetRecalculatedEffectValues( buffEffects, parameter, curTimeInMillis );

	bool updated = false;

	unsigned long long lastUpdateTime    = parameter.lastUpdateTimeInMillis / 1000;
	int                curUpdateDuration = parameter.updateDuration;
	int                curUpdateValue    = parameter.updateValue;
	int                curMaxValue       = parameter.maxValue;

	for ( const auto& entry : buffEffects )
	{
		unsigned long long eachCurTimeInMillis = std::min( curTimeInMillis, entry.first );

		curUpdateDuration = _GetEffectAddedValue(
			parameter.updateDuration,
			entry.second,
			parameter.effectIdForDuration );
		curUpdateValue = _GetEffectAddedValue(
			parameter.updateValue,
			entry.second,
			parameter.effectIdForUpdateValue );
		curMaxValue = _GetEffectAddedValue(
			parameter.maxValue,
			entry.second,
			parameter.effectIdForMaxValue );

		int duration;
		if ( parameter.lastUpdateTimeInMillis < 0 ) // 0000-00-00 00:00:00 일때
			duration = (int)( eachCurTimeInMillis / 1000 );
		else
			duration = (int)( eachCurTimeInMillis / 1000 - lastUpdateTime );

		int elapsedSeconds = curUpdateDuration - parameter.remainingSeconds;
		if ( elapsedSeconds < 0 )
			elapsedSeconds = 0;

		duration += elapsedSeconds;

		// 나머지 갱신
		int updateCount = duration / curUpdateDuration;
		_IncreaseCurValue( parameter, curUpdateValue * updateCount, curMaxValue );

		// 마지막 갱신 시각 계산
		lastUpdateTime = eachCurTimeInMillis / 1000;
		parameter.remainingSeconds = curUpdateDuration - (duration - curUpdateDuration * updateCount);
		if ( parameter.remainingSeconds == 0 )
			parameter.remainingSeconds = curUpdateDuration;

		// 버프로 증가된 현재값 다시 빼줌
		auto iter = entry.second.find( parameter.effectIdForCurValue );
		if ( iter != entry.second.end() )
		{
			int effectValue = iter->second;
			parameter.curValue = std::max( parameter.minValue, parameter.curValue - effectValue );
		}
		updated = true;
	}

	parameter.maxValue       = curMaxValue;
	parameter.updateDuration = curUpdateDuration;
	parameter.curValue       = std::max( parameter.curValue, parameter.minValue );

	if ( parameter.curValue >= parameter.maxValue )
	{
		parameter.lastUpdateTimeInMillis = curTimeInMillis;
		parameter.remainingSeconds = curUpdateDuration - 1;
		updated = true;
	}
	else
	{
		parameter.lastUpdateTimeInMillis = lastUpdateTime * 1000;
	}

	return updated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	현재 시각(밀리초)을 반환한다.
/// 
/// @param	parameter	파라미터
/// 
/// @return	현재 시각(밀리초)
////////////////////////////////////////////////////////////////////////////////////////////////////
int TimeValueUpdater::_GetCurTimeInMillis( Parameter& parameter )
{
	if ( parameter.curTimeInMillis > 0 )
		return (parameter.curTimeInMillis / 1000) * 1000;

	return time( nullptr ) * 1000;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	재계산된 이펙트 값 목록을 반환한다.
/// 
/// @param	dstTimeEffectValues	효과값 목록
/// @param	parameter			파라미터
/// @param	curTimeInMillis		현재 시각
/// 
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
void TimeValueUpdater::_GetRecalculatedEffectValues(
	TimeEffectValueList& dstTimeEffectValues,
	Parameter&           parameter,
	long long            curTimeInMillis )
{
	_GatherEffectValues( dstTimeEffectValues, parameter );

	dstTimeEffectValues.insert( std::make_pair( curTimeInMillis, EffectValueList() ) );

	_AccumulateEffectValues( dstTimeEffectValues, parameter.effectIdForCurValue );

	while ( !dstTimeEffectValues.empty() && dstTimeEffectValues.begin()->first < parameter.lastUpdateTimeInMillis )
		dstTimeEffectValues.erase( dstTimeEffectValues.begin() );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	이펙트가 추가된 값을 반환한다.
/// 
/// @param	originalValue	원본 값
/// @param	effectValues	이펙트 값 목록
/// @param	effectId		이펙트 식별자
/// 
/// @return	이펙트가 추가된 값
////////////////////////////////////////////////////////////////////////////////////////////////////
int TimeValueUpdater::_GetEffectAddedValue(
	      int              originalValue,
	const EffectValueList& effectValues,
	      int              effectId )
{
	auto iter = effectValues.find( effectId );
	if ( iter == effectValues.end() )
		return originalValue;

	return std::max( 1, originalValue + iter->second );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	파라미터를 보정한다.
/// 
/// @param	parameter	파라미터
/// 
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
void TimeValueUpdater::_CorrectParameter( Parameter& parameter )
{
	if ( parameter.updateDuration <= 0 )
		parameter.updateDuration = 1;

	if ( parameter.updateValue <= 0 )
		parameter.updateValue = 1;

	if ( parameter.remainingSeconds == -1 )
		parameter.remainingSeconds = parameter.updateDuration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	현재값을 증가시킨다.
/// 
/// @param	parameter		파라미터
/// @param	value			값
/// @param	curMaxValue		현재 최대값
/// 
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
void TimeValueUpdater::_IncreaseCurValue( Parameter& parameter, int value, int curMaxValue )
{
	if ( !parameter.downwardCorrection && parameter.curValue >= curMaxValue )
		return;

	parameter.curValue += value;
	parameter.curValue = std::min( parameter.curValue, curMaxValue );
	parameter.curValue = std::max( parameter.curValue, parameter.minValue );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	이펙트 수치가 여러 맵에 나누어져 있는 것을 하나의 맵으로 모은다.
/// 
/// @param	dstTimeEffectValues		하나로 모아서 저장될 시간 별 이펙트 수치 목록 맵
/// @param	parameter				파라미터
/// 
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
void TimeValueUpdater::_GatherEffectValues( TimeEffectValueList& dstTimeEffectValues, Parameter& parameter )
{
	if ( parameter.buffInfos.empty() )
		return;

	for ( const BuffInfo& eachBuffInfo : parameter.buffInfos )
	{
		// 현재 값에 영향을 미치는 이펙트 정보 보관
		_GatherEffectValue(
			dstTimeEffectValues,
			eachBuffInfo,
			parameter.effectIdForCurValue );

		// 최대 값에 영향을 미치는 이펙트 정보 보관
		_GatherEffectValue(
			dstTimeEffectValues,
			eachBuffInfo,
			parameter.effectIdForMaxValue );

		// 갱신 소요시간에 영향을 미치는 이펙트 정보 보관
		_GatherEffectValue(
			dstTimeEffectValues,
			eachBuffInfo,
			parameter.effectIdForDuration );

		// 갱신 값에 영향을 미치는 이펙트 정보 보관
		_GatherEffectValue(
			dstTimeEffectValues,
			eachBuffInfo,
			parameter.effectIdForUpdateValue );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	effectValues 내의 effectId에 해당하는 정보를 targetTimeEffectValues에 모은다.
/// 
/// @param	dstTimeEffectValues		중첩된 정보를 넣을 시간 별 이펙트 수치 목록
/// @param	buffInfo				버프 정보
/// @param	effectId				이펙트 식별자
///	
///	@return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
void TimeValueUpdater::_GatherEffectValue(
	      TimeEffectValueList& dstTimeEffectValues,
	const BuffInfo&            buffInfo,
	      int                  effectId )
{
	if ( buffInfo.effectId != effectId )
		return;

	EffectValueList& dstEffectValues = dstTimeEffectValues[ buffInfo.endTimeInMillis ];
	int&             dstEffectValue  = dstEffectValues[ effectId ];
	dstEffectValue += buffInfo.effectValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	이펙트 값들을 누적시킨다.
/// 
/// @param	timeEffectValues		이펙트 값 목록
/// @param	effectIdForCurValue		현재값에 대한 이펙트 식별자
/// 
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
void TimeValueUpdater::_AccumulateEffectValues( TimeEffectValueList& timeEffectValues, int effectIdForCurValue )
{
	if ( timeEffectValues.empty() )
		return;

	EffectValueList effectValues;

	// 시간의 흐름 상 앞 쪽은 뒤쪽의 이펙트들의 영향까지 같이 받는다. 누적된 값들을 계산한다.
	for ( auto iter = timeEffectValues.rbegin(); iter != timeEffectValues.rend(); iter++ )
	{
		long long        eachTimeInMillis = iter->first;
		EffectValueList& eachEffectValues = iter->second;
		for ( auto& entry : eachEffectValues )
		{
			// 현재값에 대한 이펙트 식별자를 처리하지 않는 이유는
			// 이 함수 자체가 현재값을 바꾸기 위한거라
			// 절대 계산으로 현재값을 설정하지 못하기 때문
			if ( entry.first == effectIdForCurValue )
				continue;

			int& effectValue = effectValues[ entry.first ];
			entry.second += effectValue;
			effectValue += entry.second;
		}

		eachEffectValues = effectValues;
	}
}
