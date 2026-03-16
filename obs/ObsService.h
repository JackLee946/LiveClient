#pragma once

#include <string>
#include <obs.h>

class CObsService
{
public:
    CObsService();
    ~CObsService();

    bool Create(const char* server, const char* key);
    void Update(const char* server, const char* key);

    obs_service_t* GetService() const { return m_service; }

    void Release();

private:
    obs_service_t* m_service;
};
