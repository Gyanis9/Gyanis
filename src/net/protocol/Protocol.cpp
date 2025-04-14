#include "net/protocol/Protocol.h"


namespace Gyanis::net::protocol
{
    std::shared_ptr<stream::ByteArray> Message::toByteArray()
    {
        if (auto bytearray = std::make_shared<stream::ByteArray>(); serializeToByteArray(bytearray))
        {
            return bytearray;
        }
        return nullptr;
    }

    Request::Request() : m_sn(0), m_cmd(0)
    {
    }

    uint32_t Request::getSn() const
    {
        return m_sn;
    }

    uint32_t Request::getCmd() const
    {
        return m_cmd;
    }

    void Request::setSn(const uint32_t value)
    {
        m_sn = value;
    }

    void Request::serCmd(const uint32_t value)
    {
        m_cmd = value;
    }

    bool Request::serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        byteArray->writeFuint8(getType());
        byteArray->writeUint32(m_sn);
        byteArray->writeUint32(m_cmd);
        return true;
    }

    bool Request::parseFromByteArray(const std::shared_ptr<stream::ByteArray>& bytearray)
    {
        m_sn = bytearray->readUint32();
        m_cmd = bytearray->readUint32();
        return true;
    }

    Response::Response() : m_sn(0), m_cmd(0), m_result(404), m_resultStr("unknow Handle")
    {
    }

    void Response::setSn(const uint32_t Sn)
    {
        m_sn = Sn;
    }

    void Response::setCmd(const uint32_t Cmd)
    {
        m_cmd = Cmd;
    }

    void Response::setResult(const uint32_t Result)
    {
        m_result = Result;
    }

    void Response::setResultStr(const std::string& ResultStr)
    {
        m_resultStr = ResultStr;
    }

    uint32_t Response::getSn() const
    {
        return m_sn;
    }

    uint32_t Response::getCmd() const
    {
        return m_cmd;
    }

    uint32_t Response::getResult() const
    {
        return m_result;
    }

    const std::string& Response::getResultStr() const
    {
        return m_resultStr;
    }

    bool Response::serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        byteArray->writeFuint8(getType());
        byteArray->writeUint32(m_sn);
        byteArray->writeUint32(m_cmd);
        byteArray->writeUint32(m_result);
        byteArray->writeStringVint(m_resultStr);
        return true;
    }

    bool Response::parseFromByteArray(const std::shared_ptr<stream::ByteArray>& bytearray)
    {
        m_sn = bytearray->readUint32();
        m_cmd = bytearray->readUint32();
        m_result = bytearray->readUint32();
        m_resultStr = bytearray->readStringVint();
        return true;
    }

    Notify::Notify() : m_notify(0)
    {
    }

    void Notify::setNotify(const uint32_t notify)
    {
        m_notify = notify;
    }

    uint32_t Notify::getNotify() const
    {
        return m_notify;
    }

    bool Notify::serializeToByteArray(const std::shared_ptr<stream::ByteArray>& byteArray)
    {
        byteArray->writeFuint8(getType());
        byteArray->writeUint32(m_notify);
        return true;
    }

    bool Notify::parseFromByteArray(const std::shared_ptr<stream::ByteArray>& bytearray)
    {
        m_notify = bytearray->readUint32();
        return true;
    }
}
