#include "net/http/servlets/ConfigServlet.h"
#include "base/Config.h"
#include "base/JsonUtils.h"

namespace Gyanis::net::http
{
    ConfigServlet::ConfigServlet(): Servlet("ConfigServlet")
    {
    }

    int32_t ConfigServlet::handle(const std::shared_ptr<HttpRequest>& request,
                                  const std::shared_ptr<HttpResponse>& response,
                                  const std::shared_ptr<HttpSession>& session)
    {
        const std::string type = request->getParam("type");
        if (type == "json")
        {
            response->setHeader("Content-Type", "text/json charset=utf-8");
        }
        else
        {
            response->setHeader("Content-Type", "text/yaml charset=utf-8");
        }
        YAML::Node node;
        base::Config::Visit([&node](const std::shared_ptr<base::ConfigVarBase>& base)
        {
            YAML::Node n;
            try
            {
                n = YAML::Load(base->toString());
            }
            catch (...)
            {
                return;
            }
            node[base->getName()] = n;
            node[base->getName() + "->description"] = base->getDescription();
        });
        if (type == "json")
        {
            if (nlohmann::json jvalue; base::YamlToJson(node, jvalue))
            {
                response->setBody(base::JsonUtils::ToString(jvalue));
                return 0;
            }
        }
        std::stringstream ss;
        ss << node;
        response->setBody(ss.str());
        return 0;
    }
}
