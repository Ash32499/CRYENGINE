// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Cue.h"
#include "BaseObject.h"
#include "CueInstance.h"
#include "Impl.h"
#include "Listener.h"

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include <Logger.h>
#endif // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
ERequestStatus CCue::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject*>(pIObject);

		switch (m_actionType)
		{
		case EActionType::Start:
			{
				CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();

				criAtomExPlayer_Set3dListenerHn(pPlayer, g_pListener->GetHandle());
				criAtomExPlayer_Set3dSourceHn(pPlayer, pBaseObject->Get3dSource());

				auto const iter = g_acbHandles.find(m_cueSheetId);

				if (iter != g_acbHandles.end())
				{
					CriAtomExAcbHn const acbHandle = iter->second;
					auto const cueName = static_cast<CriChar8 const*>(m_name);

					criAtomExPlayer_SetCueName(pPlayer, acbHandle, cueName);
					CriAtomExPlaybackId const playbackId = criAtomExPlayer_Start(pPlayer);

					while (true)
					{
						// Loop is needed because callbacks don't work for cues that fail to start.
						CriAtomExPlaybackStatus const status = criAtomExPlayback_GetStatus(playbackId);

						if (status != CRIATOMEXPLAYBACK_STATUS_PREP)
						{
							if (status == CRIATOMEXPLAYBACK_STATUS_PLAYING)
							{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
								auto const pCueInstance = g_pImpl->ConstructCueInstance(triggerInstanceId, m_id, playbackId, pBaseObject, this);
#else
								auto const pCueInstance = g_pImpl->ConstructCueInstance(triggerInstanceId, m_id, playbackId);
#endif                // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

								CriAtomExCueInfo cueInfo;

								if (criAtomExAcb_GetCueInfoByName(acbHandle, cueName, &cueInfo) == CRI_TRUE)
								{
									if (cueInfo.pos3d_info.doppler_factor > 0.0f)
									{
										pCueInstance->SetFlag(ECueInstanceFlags::HasDoppler);
									}
								}

								if (criAtomExAcb_IsUsingAisacControlByName(acbHandle, cueName, g_szAbsoluteVelocityAisacName) == CRI_TRUE)
								{
									pCueInstance->SetFlag(ECueInstanceFlags::HasAbsoluteVelocity);
								}

								pBaseObject->AddCueInstance(pCueInstance);
								pBaseObject->UpdateVelocityTracking();

								requestResult = ((pCueInstance->GetFlags() & ECueInstanceFlags::IsVirtual) != 0) ? ERequestStatus::SuccessVirtual : ERequestStatus::Success;
							}

							break;
						}
					}
				}
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
				else
				{
					Cry::Audio::Log(ELogType::Warning, R"(Cue "%s" failed to play because ACB file "%s" was not loaded)",
					                static_cast<char const*>(m_name), static_cast<char const*>(m_cueSheetName));
				}
#endif        // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

				break;
			}
		case EActionType::Stop:
			{
				pBaseObject->StopCue(m_id);
				requestResult = ERequestStatus::SuccessDoNotTrack;

				break;
			}
		case EActionType::Pause:
			{
				pBaseObject->PauseCue(m_id);
				requestResult = ERequestStatus::SuccessDoNotTrack;
				break;
			}
		case EActionType::Resume:
			{
				pBaseObject->ResumeCue(m_id);
				requestResult = ERequestStatus::SuccessDoNotTrack;

				break;
			}
		default:
			{
				break;
			}
		}
	}
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid object pointer passed to the Adx2 implementation of %s.", __FUNCTION__);
	}
#endif        // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
void CCue::Stop(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
		pBaseObject->StopCue(m_id);
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
