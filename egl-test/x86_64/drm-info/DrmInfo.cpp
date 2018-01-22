#include "DrmInfo.h"

#include <fcntl.h>
#include <zconf.h>
#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;
using namespace Drm;

static const char BaseDeviceName[] = "/dev/dri/card";

static const size_t InvalidNumCards = size_t(-1);
size_t CardsInfo::_numCards = InvalidNumCards;

string CardsInfo::GetDeviceName(size_t cardNumber)
{
    ostringstream stream;
//    stream << "/dev/dri/renderD" << cardNumber + 128;
    stream << BaseDeviceName << cardNumber;
    return stream.str();
}

size_t CardsInfo::GetNumCards()
{
    if (_numCards != InvalidNumCards)
        return _numCards;
    size_t index = 0;
    bool success = true;
    while (success)
    {
        CardInfo info = CardInfo::OpenCard(index);
        success = info.IsOpen();
        if (success)
            ++index;
    }
    return index;
}

VideoMode::VideoMode()
    : _valid(false)
    , info()
{}

VideoMode::VideoMode(const drmModeModeInfo & value)
    : _valid(true)
    , info(value)
{}

Property::Property()
    : _valid(false)
    , id()
    , value()
{}

Property::Property(PropertyID aID, PropertyValue aValue)
    : _valid(true)
    , id(aID)
    , value(aValue)
{}

FrameBufferInfo::FrameBufferInfo(const drmModeFB & value)
    : _info(value)
{
}

CRTCInfo::CRTCInfo(const drmModeCrtc & value)
    : _info(value)
{
}

EncoderInfo::EncoderInfo(CardInfo & cardInfo, const drmModeEncoder & value)
    : _info(value)
    , _crtc(cardInfo.GetCRTC(value.crtc_id))
{
}

const CRTCInfo * EncoderInfo::GetCrtc() const
{
    return _crtc;
}

ConnectorInfo::ConnectorInfo(CardInfo & cardInfo, const drmModeConnector & connector)
    : _id(connector.connector_id)
    , _connectedEncoderID(connector.encoder_id)
    , _type(connector.connector_type)
    , _typeID(connector.connector_type_id)
    , _connectionState(static_cast<ConnectionState>(connector.connection))
    , _geometryMM(connector.mmWidth, connector.mmHeight)
    , _subPixelMode(static_cast<SubPixelMode>(connector.subpixel))
    , _videoModes()
    , _encoders()
    , _properties()
{
    for (int i = 0; i < connector.count_modes; ++i)
    {
        _videoModes.emplace_back(connector.modes[i]);
    }
    for (int i = 0; i < connector.count_encoders; ++i)
    {
        _encoders.emplace_back(cardInfo.GetEncoder(connector.encoders[i]));
    }
    for (int i = 0; i < connector.count_props; ++i)
    {
        _properties.insert({connector.props[i], connector.prop_values[i]});
    }
}

const VideoMode & ConnectorInfo::GetVideoMode(size_t index) const
{
    assert((index >= 0) && (index < GetNumVideoModes()));
    return _videoModes[index];
}

const EncoderInfo * ConnectorInfo::GetEncoder(size_t id) const
{
    for (auto const & encoder : _encoders)
    {
        if (encoder->GetID() == id)
            return encoder;
    }
    ostringstream stream;
    stream << "Requested Encoder with invalid id: " << id << ".";
    Log(stream.str());
    return nullptr;
}

const EncoderInfo * ConnectorInfo::GetEncoderByIndex(size_t index) const
{
    assert((index >= 0) && (index < GetNumEncoders()));
    return _encoders[index];
}

bool ConnectorInfo::HaveProperty(uint32_t id, uint64_t & value) const
{
    auto it = _properties.find(id);
    if (it != _properties.end())
    {
        value = it->second;
        return true;
    }
    return false;
}

uint64_t ConnectorInfo::GetProperty(uint32_t id) const
{
    uint64_t value;
    bool haveProperty = HaveProperty(id, value);
    assert(haveProperty);
    return value;
}

Property ConnectorInfo::GetPropertyByIndex(size_t index) const
{
    assert((index >= 0) && (index < _properties.size()));
    auto it = _properties.begin();
    for (size_t i = 0; i < index; ++i)
        ++it;
    return Property{it->first, it->second};
}

Rect ConnectorInfo::GetRect() const
{
    if (IsConnected())
    {
        auto encoder = GetEncoder(GetConnectedEncoderID());
        auto crtc = encoder->GetCrtc();
        assert(crtc != nullptr);
        // If the origin is (0,0), we expect it to be the primary
        return Rect(crtc->_info.x, crtc->_info.y, crtc->_info.width, crtc->_info.height);
    }
    return Rect(-1, -1, -1, -1);
}

CardDescriptor::CardDescriptor()
    : _fd(-1)
{
}

CardDescriptor::CardDescriptor(const std::string & deviceName)
    : _fd(-1)
{
    Open(deviceName);
}

CardDescriptor::CardDescriptor(CardDescriptor && other) noexcept
    : _fd(std::move(other._fd))
{
    other._fd = -1;
}

CardDescriptor::~CardDescriptor()
{
    Close();
}

CardDescriptor & CardDescriptor::operator = (CardDescriptor && other) noexcept
{
    if (this != &other)
    {
        _fd = std::move(other._fd);
        other._fd = -1;
    }
    return *this;
}

bool CardDescriptor::Open(const std::string & deviceName)
{
    _fd = open(deviceName.c_str(), O_RDWR);
    if (!IsOpen())
    {
        Close();
        return false;
    }
    return true;
}

void CardDescriptor::Close()
{
    if (IsOpen())
        close(_fd);
    _fd = -1;
}

bool CardDescriptor::IsOpen() const
{
    return (_fd >= 0);
}

CardInfo CardInfo::OpenCard(size_t cardNumber)
{
    return CardInfo(CardsInfo::GetDeviceName(cardNumber));
}

CardInfo::CardInfo()
    : CardDescriptor()
    , _connectors()
    , _frameBuffers()
    , _crtcs()
    , _encoders()
    , _minSize()
    , _maxSize()
{
}

CardInfo::CardInfo(const std::string & deviceName)
    : CardDescriptor(deviceName)
    , _connectors()
    , _frameBuffers()
    , _crtcs()
    , _encoders()
    , _minSize()
    , _maxSize()
{
    Open(deviceName);
}

CardInfo::CardInfo(CardInfo && other) noexcept
    : CardDescriptor(std::move(other))
    , _connectors(std::move(other._connectors))
    , _frameBuffers(std::move(other._frameBuffers))
    , _crtcs(std::move(other._crtcs))
    , _encoders(std::move(other._encoders))
    , _minSize(std::move(other._minSize))
    , _maxSize(std::move(other._maxSize))
{
    other._minSize = {};
    other._maxSize = {};
}

CardInfo::~CardInfo()
{
    Close();
}

CardInfo & CardInfo::operator = (CardInfo && other) noexcept
{
    if (this != &other)
    {
        CardDescriptor::operator = (std::move(other));
        _connectors = std::move(other._connectors);
        _frameBuffers = std::move(other._frameBuffers);
        _crtcs = std::move(other._crtcs);
        _encoders = std::move(other._encoders);
        _minSize = std::move(other._minSize);
        _maxSize = std::move(other._maxSize);
        other._minSize = {};
        other._maxSize = {};
    }
    return *this;
}

bool CardInfo::Open(const std::string & deviceName)
{
    if (!CardDescriptor::Open(deviceName))
    {
        Close();
        return false;
    }
    drmModeRes * drmModeResources = drmModeGetResources(_fd);
    if (!drmModeResources)
    {
        Log("Cannot get drm resources");
        return false;
    }
    for (int i = 0; i < drmModeResources->count_fbs; ++i)
    {
        auto frameBuffer = drmModeGetFB(_fd, drmModeResources->fbs[i]);
        if (frameBuffer)
        {
            _frameBuffers.emplace_back(*frameBuffer);
            drmModeFreeFB(frameBuffer);
        }
    }
    for (int i = 0; i < drmModeResources->count_crtcs; ++i)
    {
        auto crtc = drmModeGetCrtc(_fd, drmModeResources->crtcs[i]);
        if (crtc)
        {
            _crtcs.emplace_back(*crtc);
            drmModeFreeCrtc(crtc);
        }
    }
    for (int i = 0; i < drmModeResources->count_encoders; ++i)
    {
        auto encoder = drmModeGetEncoder(_fd, drmModeResources->encoders[i]);
        if (encoder)
        {
            _encoders.emplace_back(*this, *encoder);
            drmModeFreeEncoder(encoder);
        }
    }
    for (int i = 0; i < drmModeResources->count_connectors; ++i)
    {
        auto connector = drmModeGetConnector(_fd, drmModeResources->connectors[i]);
        if (connector)
        {
            _connectors.emplace_back(*this, *connector);
            drmModeFreeConnector(connector);
        }
    }
    _minSize = { drmModeResources->min_width, drmModeResources->min_height };
    _maxSize = { drmModeResources->max_width, drmModeResources->max_height };
    drmModeFreeResources(drmModeResources);
    return true;
}

void CardInfo::Close()
{
    CardDescriptor::Close();
    _connectors.clear();
    _encoders.clear();
    _minSize = {};
    _maxSize = {};
}

const ConnectorInfo * CardInfo::GetConnector(size_t id) const
{
    for (auto const & connector : _connectors)
    {
        if (connector.GetID() == id)
            return &connector;
    }
    ostringstream stream;
    stream << "Requested Connector with invalid id: " << id << ".";
    Log(stream.str());
    return nullptr;
}

const ConnectorInfo * CardInfo::GetConnectorByIndex(size_t index) const
{
    if ((index >= 0) && (index < GetNumConnectors()))
        return &_connectors[index];
    ostringstream stream;
    stream << "Requested Connector with invalid index: " << index << ". Only index 0 through " << GetNumConnectors() << " are supported.";
    Log(stream.str());
    return nullptr;
}

const ConnectorInfo * CardInfo::GetPrimaryConnector() const
{
    for (auto const & connector : _connectors)
    {
        if (connector.IsConnected())
        {
            const Rect & rc = connector.GetRect();
            // If the origin is (0,0), we expect it to be the primary
            if ((rc.x == 0) && (rc.y == 0))
                return &connector;
        }
    }
    return nullptr;
}

std::vector<const ConnectorInfo *> CardInfo::GetActiveConnectors() const
{
    std::vector<const ConnectorInfo *> result;
    for (auto const & connector : _connectors)
    {
        if (connector.IsConnected())
        {
            result.push_back(&connector);
        }
    }
    return result;
}

const FrameBufferInfo * CardInfo::GetFrameBuffer(size_t id) const
{
    for (auto const & frameBuffer : _frameBuffers)
    {
        if (frameBuffer._info.fb_id == id)
            return &frameBuffer;
    }
    ostringstream stream;
    stream << "Requested FrameBuffer with invalid id: " << id << ".";
    Log(stream.str());
    return nullptr;
}

const FrameBufferInfo * CardInfo::GetFrameBufferByIndex(size_t index) const
{
    if ((index >= 0) && (index < GetNumFrameBuffers()))
        return &_frameBuffers[index];
    ostringstream stream;
    stream << "Requested FrameBuffer with invalid index: " << index << ". Only index 0 through " << GetNumFrameBuffers() << " are supported.";
    Log(stream.str());
    return nullptr;
}

const CRTCInfo * CardInfo::GetCRTC(size_t id) const
{
    if (id == 0)
        return nullptr;
    for (auto const & crtc : _crtcs)
    {
        if (crtc._info.crtc_id == id)
            return &crtc;
    }
    ostringstream stream;
    stream << "Requested CRTC with invalid id: " << id << ".";
    Log(stream.str());
    return nullptr;
}

const CRTCInfo * CardInfo::GetCRTCByIndex(size_t index) const
{
    if ((index >= 0) && (index < GetNumCRTCs()))
        return &_crtcs[index];
    ostringstream stream;
    stream << "Requested CRTC with invalid index: " << index << ". Only index 0 through " << GetNumCRTCs() << " are supported.";
    Log(stream.str());
    return nullptr;
}

const EncoderInfo * CardInfo::GetEncoder(size_t id) const
{
    for (auto const & encoder : _encoders)
    {
        if (encoder.GetID() == id)
            return &encoder;
    }
    ostringstream stream;
    stream << "Requested Encoder with invalid id: " << id << ".";
    Log(stream.str());
    return nullptr;
}

const EncoderInfo * CardInfo::GetEncoderByIndex(size_t index) const
{
    if ((index >= 0) && (index < GetNumEncoders()))
        return &_encoders[index];
    ostringstream stream;
    stream << "Requested Encoder with invalid index: " << index << ". Only index 0 through " << GetNumEncoders() << " are supported.";
    Log(stream.str());
    return nullptr;
}

void Drm::Log(const std::string & message)
{
    cerr << message << endl;
}

ostream & operator << (ostream & stream, Drm::ConnectorInfo::ConnectorType value)
{
    switch (value)
    {
        case Drm::ConnectorInfo::ConnectorType::Unknown:        stream << "Unknown"; break;
        case Drm::ConnectorInfo::ConnectorType::VGA:            stream << "VGA"; break;
        case Drm::ConnectorInfo::ConnectorType::DVI_I:          stream << "DVI I"; break;
        case Drm::ConnectorInfo::ConnectorType::DVI_D:          stream << "DVI D"; break;
        case Drm::ConnectorInfo::ConnectorType::DVI_A:          stream << "DVI A"; break;
        case Drm::ConnectorInfo::ConnectorType::Composite:      stream << "Composite"; break;
        case Drm::ConnectorInfo::ConnectorType::SVideo:         stream << "S-Video"; break;
        case Drm::ConnectorInfo::ConnectorType::LVDS:           stream << "LVDS"; break;
        case Drm::ConnectorInfo::ConnectorType::Component:      stream << "Component"; break;
        case Drm::ConnectorInfo::ConnectorType::DIN9Pin:        stream << "9 pin DIN"; break;
        case Drm::ConnectorInfo::ConnectorType::DisplayPort:    stream << "DisplayPort"; break;
        case Drm::ConnectorInfo::ConnectorType::HDMI_A:         stream << "HDMI A"; break;
        case Drm::ConnectorInfo::ConnectorType::HDMI_B:         stream << "HDMI B"; break;
        case Drm::ConnectorInfo::ConnectorType::ConnectorTV:    stream << "Connector TV"; break;
        case Drm::ConnectorInfo::ConnectorType::eDP:            stream << "eDP"; break;
        default:                                                stream << "?"; break;
    }
    return stream;
}

ostream & operator << (ostream & stream, Drm::ConnectorInfo::ConnectionState value)
{
    switch (value)
    {
        case Drm::ConnectorInfo::ConnectionState::Connected:    stream << "Y"; break;
        case Drm::ConnectorInfo::ConnectionState::Disconnected: stream << "N"; break;
        case Drm::ConnectorInfo::ConnectionState::Unknown:      stream << "?"; break;
        default:                                                stream << "?"; break;
    }
    return stream;
}

ostream & operator << (ostream & stream, Drm::ConnectorInfo::SubPixelMode value)
{
    switch (value)
    {
        case Drm::ConnectorInfo::SubPixelMode::Unknown:         stream << "Unknown"; break;
        case Drm::ConnectorInfo::SubPixelMode::HorizontalRGB:   stream << "Hor RGB"; break;
        case Drm::ConnectorInfo::SubPixelMode::HorizontalBGR:   stream << "Hor BGR"; break;
        case Drm::ConnectorInfo::SubPixelMode::VerticalRGB:     stream << "Ver RGB"; break;
        case Drm::ConnectorInfo::SubPixelMode::VerticalBGR:     stream << "Ver BGR"; break;
        case Drm::ConnectorInfo::SubPixelMode::SubPixelModeNone:stream << "None"; break;
        default:                                                stream << "?"; break;
    }
    return stream;
}

ostream & operator << (ostream & stream, Drm::EncoderInfo::EncoderType value)
{
    switch (value)
    {
        case Drm::EncoderInfo::EncoderType::EncoderNone:       stream << "None"; break;
        case Drm::EncoderInfo::EncoderType::DAC:        stream << "DAC"; break;
        case Drm::EncoderInfo::EncoderType::TMDS:       stream << "TMDS"; break;
        case Drm::EncoderInfo::EncoderType::LVDS:       stream << "LVDS"; break;
        case Drm::EncoderInfo::EncoderType::TVDAC:      stream << "TV DAC"; break;
        default:                                        stream << "?"; break;
    }
    return stream;
}

