#include "engine_share.hpp"

#include <cstdint>
#include <vector>

#include "core/nerine_memory.hpp"
#include "object_array.hpp"
#include "offset_resolver.hpp"
#include "remote_object.hpp"

static uint64_t g_engineShareAddress;

int EngineShare::GetPhysicsFrame()
{
    OffsetResolver *offsetResolver = OffsetResolver::GetInstance();
    uint64_t engineShareClass      = offsetResolver->TryGetClassAddress("EngineShare_TA");
    if (!engineShareClass)
    {
        return 0;
    }

    if (!RemoteObject(g_engineShareAddress).IsValidInstanceOf(engineShareClass))
    {
        g_engineShareAddress          = 0;
        std::vector<uint64_t> objects = ObjectArray::GetAllObjectAddresses();
        for (size_t i = 0; i < objects.size(); i++)
        {
            RemoteObject object(objects[i]);
            if (object.IsValidInstanceOf(engineShareClass) && !object.IsDefaultObject())
            {
                g_engineShareAddress = object.GetAddress();
                break;
            }
        }

        if (!g_engineShareAddress)
        {
            return 0;
        }
    }

    return ReadMemory<int>(g_engineShareAddress + offsetResolver->GetPropertyOffsetCached(engineShareClass, "PhysicsFrame"));
}
