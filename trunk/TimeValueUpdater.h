////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file	TimeValueUpdater.h
///
/// @brief	TimeValueUpdater 클래스의 헤더 파일
///
/// @date	2015-7-23
////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <list>
#include <map>


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @class	TimeValueUpdater
///
/// @brief	시간 갱신에 따라 변하는 값을 처리하기 위한 클래스
////////////////////////////////////////////////////////////////////////////////////////////////////
class TimeValueUpdater
{
private:
	typedef std::map< int, int >                   EffectValueList;     ///< 효과 값 목록
	typedef std::map< long long, EffectValueList > TimeEffectValueList; ///< 시간에 따른 효과 값 목록

public:
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// @class	Info of buff
	////////////////////////////////////////////////////////////////////////////////////////////////////
	class BuffInfo
	{
	public:
		int       effectId;        ///< 이펙트 식별자
		int       effectValue;     ///< 이펙트 값
		long long endTimeInMillis; ///< 종료 시각(msec)
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// @class	파라미터
	////////////////////////////////////////////////////////////////////////////////////////////////////
	class Parameter
	{
	public:
		std::list< BuffInfo > buffInfos;              ///< 버프 정보 목록
		int                   curValue;               ///< 현재값
		int                   minValue;               ///< 최소값
		int                   maxValue;               ///< 최대값
		long long             lastUpdateTimeInMillis; ///< 마지막 갱신 시각
		long long             curTimeInMillis;        ///< 현재 시각
		int                   updateDuration;         ///< 갱신 소요 시간
		int                   updateValue;            ///< 갱신 값
		int                   effectIdForCurValue;    ///< 현재 값에 영향을 미치는 이펙트 식별자
		int                   effectIdForMaxValue;    ///< 최대 값에 영향을 미치는 이펙트 종류
		int                   effectIdForDuration;    ///< 갱신 소요시간에 영향을 미치는 이펙트 종류
		int                   effectIdForUpdateValue; ///< 갱신 값에 영향을 미치는 이펙트 종류
		int                   remainingSeconds;       ///< 남은 초
		bool                  downwardCorrection;     ///< 하향 보정 여부

		/// 생성자
		Parameter();

		/// 이 객체를 처음 상태로 되돌린다.
		void Reset();

		/// 값을 갱신한다.
		bool Update();
	};

	/// 값을 갱신한다.
	static bool Update( Parameter& parameter );

private:
	/// 현재 시각(밀리초)을 반환한다.
	static long long _GetCurTimeInMillis( Parameter& parameter );

	/// 재계산된 이펙트 값 목록을 반환한다.
	static void _GetRecalculatedEffectValues(
		TimeEffectValueList& dstTimeEffectValues,
		Parameter&           parameter,
		long long            curTimeInMillis );

	/// 이펙트가 추가된 값을 반환한다.
	static int _GetEffectAddedValue(
		      int              originalValue,
		const EffectValueList& effectValues,
		      int              effectId );

	/// 파라미터를 보정한다.
	static void _CorrectParameter( Parameter& parameter );

	/// 현재값을 증가시킨다.
	static void _IncreaseCurValue( Parameter& parameter, int value, int curMaxValue );

	/// 이펙트 수치가 여러 맵에 나누어져 있는 것을 하나의 맵으로 모은다.
	static void _GatherEffectValues( TimeEffectValueList& dstTimeEffectValues, Parameter& parameter );

	/// effectValues 내의 effectId에 해당하는 정보를 targetTimeEffectValues에 모은다.
	static void _GatherEffectValue(
		      TimeEffectValueList& dstTimeEffectValues,
		const BuffInfo&            buffInfo,
		      int                  effectId );

	/// 이펙트 값들을 누적시킨다.
	static void _AccumulateEffectValues( TimeEffectValueList& timeEffectValues, int effectIdForCurValue );
};
