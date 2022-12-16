#include "transport.h"

#include <map>

namespace mtest
{
    std::map<std::string, transport_creator> transport_creators;

    bool register_transport(const std::string& name, transport_creator&& creator)
    {
        return transport_creators.emplace(name, creator).second;
    }

    transport_creator get_transport_creator(const std::string& name)
    {
        auto it = transport_creators.find(name);
        return it != transport_creators.end() ? it->second : nullptr;
    }

    std::vector<transport_creator> get_transport_creators()
    {
        std::vector<transport_creator> res;
        for (const auto &[key, value] : transport_creators)
        {
            res.push_back(value);
        }
        return res;
    }
}//namespace mtest