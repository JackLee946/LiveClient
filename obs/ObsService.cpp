#include "stdafx.h"
#include "ObsService.h"

CObsService::CObsService()
    : m_service(nullptr)
{
}

CObsService::~CObsService()
{
    Release();
}

bool CObsService::Create(const char* server, const char* key)
{
    if (m_service)
    {
        obs_service_release(m_service);
        m_service = nullptr;
    }

    obs_data_t* settings = obs_data_create();
    obs_data_set_string(settings, "server", server ? server : "");
    obs_data_set_string(settings, "key", key ? key : "");

    m_service = obs_service_create("rtmp_custom", "rtmp_service", settings, nullptr);
    obs_data_release(settings);

    return m_service != nullptr;
}

void CObsService::Update(const char* server, const char* key)
{
    if (!m_service)
    {
        Create(server, key);
        return;
    }

    obs_data_t* settings = obs_data_create();
    obs_data_set_string(settings, "server", server ? server : "");
    obs_data_set_string(settings, "key", key ? key : "");

    obs_service_update(m_service, settings);
    obs_data_release(settings);
}

void CObsService::Release()
{
    if (m_service)
    {
        obs_service_release(m_service);
        m_service = nullptr;
    }
}
