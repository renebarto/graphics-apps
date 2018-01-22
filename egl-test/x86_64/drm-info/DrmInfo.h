#pragma once

#include <string>
#include <map>
#include <vector>
#include <xf86drmMode.h>

namespace Drm
{

using ConnectorID = uint32_t;
using EncoderID = uint32_t;
using CrtcID = uint32_t;

class CardsInfo
{
public:
    static size_t GetNumCards();
    static std::string GetDeviceName(size_t cardNumber);

protected:
    static size_t _numCards;
};

struct Geometry
{
    Geometry() = default;
    Geometry(uint32_t aWidth, uint32_t aHeight)
        : width(aWidth)
        , height(aHeight)
    {}
    uint32_t width;
    uint32_t height;
};

struct Rect
{
    Rect() = default;
    Rect(uint32_t aX, uint32_t aY, uint32_t aWidth, uint32_t aHeight)
        : x(aX)
        , y(aY)
        , width(aWidth)
        , height(aHeight)
    {}
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

class VideoMode
{
public:
    VideoMode();
    explicit VideoMode(const drmModeModeInfo & value);

    const drmModeModeInfo info;

    operator bool () const {return _valid; }

protected:
    bool _valid;
};

using PropertyID = uint32_t;
using PropertyValue = uint64_t;

struct Property
{
    Property();
    explicit Property(PropertyID aID, PropertyValue aValue);

    const PropertyID id;
    PropertyValue value;

    operator bool () const {return _valid; }

protected:
    bool _valid;
};

class FrameBufferInfo
{
public:
    explicit FrameBufferInfo(const drmModeFB & value);

    const drmModeFB _info;
};

class CRTCInfo
{
public:
    explicit CRTCInfo(const drmModeCrtc & value);

    const drmModeCrtc _info;
};

class CardInfo;

class EncoderInfo
{
public:
    enum class EncoderType
    {
        EncoderNone = DRM_MODE_ENCODER_NONE,
        DAC = DRM_MODE_ENCODER_DAC,
        TMDS = DRM_MODE_ENCODER_TMDS,
        LVDS = DRM_MODE_ENCODER_LVDS,
        TVDAC = DRM_MODE_ENCODER_TVDAC,
    };
    explicit EncoderInfo(CardInfo & cardInfo, const drmModeEncoder & value);

    EncoderID GetID() const { return _info.encoder_id; }
    EncoderType GetType() const { return static_cast<EncoderType>(_info.encoder_type); }
    CrtcID GetCrtcID() const { return _info.crtc_id; }
    const CRTCInfo * GetCrtc() const;
    size_t GetNumPossibleClones() const { return _info.possible_clones; }
    size_t GetNumPossibleCrtcs() const { return _info.possible_crtcs; }

protected:
    const drmModeEncoder _info;
    const CRTCInfo * _crtc;
};

class ConnectorInfo
{
public:
    enum class ConnectionState
    {
        Connected = drmModeConnection::DRM_MODE_CONNECTED,
        Disconnected = drmModeConnection::DRM_MODE_DISCONNECTED,
        Unknown = drmModeConnection::DRM_MODE_UNKNOWNCONNECTION,
    };
    enum class SubPixelMode
    {
        Unknown = drmModeSubPixel::DRM_MODE_SUBPIXEL_UNKNOWN,
        HorizontalRGB = drmModeSubPixel::DRM_MODE_SUBPIXEL_HORIZONTAL_RGB,
        HorizontalBGR = drmModeSubPixel::DRM_MODE_SUBPIXEL_HORIZONTAL_BGR,
        VerticalRGB = drmModeSubPixel::DRM_MODE_SUBPIXEL_VERTICAL_RGB,
        VerticalBGR = drmModeSubPixel::DRM_MODE_SUBPIXEL_VERTICAL_BGR,
        SubPixelModeNone = drmModeSubPixel::DRM_MODE_SUBPIXEL_NONE,
    };
    enum class ConnectorType
    {
        Unknown = DRM_MODE_CONNECTOR_Unknown,
        VGA = DRM_MODE_CONNECTOR_VGA,
        DVI_I = DRM_MODE_CONNECTOR_DVII,
        DVI_D = DRM_MODE_CONNECTOR_DVID,
        DVI_A = DRM_MODE_CONNECTOR_DVIA,
        Composite = DRM_MODE_CONNECTOR_Composite,
        SVideo = DRM_MODE_CONNECTOR_SVIDEO,
        LVDS = DRM_MODE_CONNECTOR_LVDS,
        Component = DRM_MODE_CONNECTOR_Component,
        DIN9Pin = DRM_MODE_CONNECTOR_9PinDIN,
        DisplayPort = DRM_MODE_CONNECTOR_DisplayPort,
        HDMI_A = DRM_MODE_CONNECTOR_HDMIA,
        HDMI_B = DRM_MODE_CONNECTOR_HDMIB,
        ConnectorTV = DRM_MODE_CONNECTOR_TV,
        eDP = DRM_MODE_CONNECTOR_eDP,
    };
    using PropertyMap = std::map<PropertyID, PropertyValue>;

    ConnectorInfo(CardInfo & cardInfo, const drmModeConnector & connector);

    ConnectorID GetID() const { return _id; }
    ConnectorType GetType() const { return static_cast<ConnectorType>(_type); }
    uint32_t GetTypeID() const { return _typeID; }
    bool IsConnected() const { return _connectionState == ConnectionState::Connected; }
    ConnectionState GetConnectionState() const { return _connectionState; }
    Geometry GetGeometryMM() const { return _geometryMM; }
    SubPixelMode GetSubPixelMode() const { return _subPixelMode; }

    size_t GetNumVideoModes() const { return _videoModes.size(); }
    const VideoMode & GetVideoMode(size_t index) const;

    EncoderID GetConnectedEncoderID() const { return _connectedEncoderID; }
    size_t GetNumEncoders() const { return _encoders.size(); }
    const EncoderInfo * GetEncoder(size_t id) const;
    const EncoderInfo * GetEncoderByIndex(size_t index) const;

    size_t GetNumProperties() const { return _properties.size(); }
    bool HaveProperty(PropertyID id, PropertyValue & value) const;
    PropertyValue GetProperty(PropertyID id) const;
    Property GetPropertyByIndex(size_t index) const;

    Rect GetRect() const;

private:
    ConnectorID _id;
    uint32_t _connectedEncoderID;
    uint32_t _type;
    uint32_t _typeID;
    ConnectionState _connectionState;
    Geometry _geometryMM;
    SubPixelMode _subPixelMode;
    std::vector<VideoMode> _videoModes;
    std::vector<const EncoderInfo *> _encoders;
    PropertyMap _properties;
};

class CardDescriptor
{
public:
    CardDescriptor();
    explicit CardDescriptor(const std::string & deviceName);
    CardDescriptor(const CardDescriptor & other) = delete;
    CardDescriptor(CardDescriptor && other) noexcept;
    virtual ~CardDescriptor();
    CardDescriptor & operator = (const CardDescriptor & other) = delete;
    CardDescriptor & operator = (CardDescriptor && other) noexcept;

    virtual bool Open(const std::string & deviceName);
    virtual void Close();
    bool IsOpen() const;

    int GetFD() const { return _fd; }

protected:
    int _fd;
};

class CardInfo : public CardDescriptor
{
public:
    static CardInfo OpenCard(size_t cardNumber);

    CardInfo();
    explicit CardInfo(const std::string & deviceName);
    CardInfo(const CardInfo & other) = delete;
    CardInfo(CardInfo && other) noexcept;
    ~CardInfo() final;
    CardInfo & operator = (const CardInfo & other) = delete;
    CardInfo & operator = (CardInfo && other) noexcept;

    bool Open(const std::string & deviceName) override;
    void Close() override;

    size_t GetNumConnectors() const { return _connectors.size(); }
    const ConnectorInfo * GetConnector(size_t id) const;
    const ConnectorInfo * GetConnectorByIndex(size_t index) const;
    const ConnectorInfo * GetPrimaryConnector() const;
    std::vector<const ConnectorInfo *> GetActiveConnectors() const;

    size_t GetNumFrameBuffers() const { return _frameBuffers.size(); }
    const FrameBufferInfo * GetFrameBuffer(size_t id) const;
    const FrameBufferInfo * GetFrameBufferByIndex(size_t index) const;

    size_t GetNumCRTCs() const { return _crtcs.size(); }
    const CRTCInfo * GetCRTC(size_t id) const;
    const CRTCInfo * GetCRTCByIndex(size_t index) const;

    size_t GetNumEncoders() const { return _encoders.size(); }
    const EncoderInfo * GetEncoder(size_t id) const;
    const EncoderInfo * GetEncoderByIndex(size_t index) const;
    Geometry GetMinSizePixels() const { return _minSize; }
    Geometry GetMaxSizePixels() const { return _maxSize; }

protected:
    std::vector<ConnectorInfo> _connectors;
    std::vector<FrameBufferInfo> _frameBuffers;
    std::vector<CRTCInfo> _crtcs;
    std::vector<EncoderInfo> _encoders;
    Geometry _minSize;
    Geometry _maxSize;
};

void Log(const std::string & message);

} // namespace Drm

std::ostream & operator << (std::ostream & stream, Drm::ConnectorInfo::ConnectorType value);
std::ostream & operator << (std::ostream & stream, Drm::ConnectorInfo::ConnectionState value);
std::ostream & operator << (std::ostream & stream, Drm::ConnectorInfo::SubPixelMode value);
std::ostream & operator << (std::ostream & stream, Drm::EncoderInfo::EncoderType value);
