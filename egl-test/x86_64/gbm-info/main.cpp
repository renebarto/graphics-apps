#include <fcntl.h>
#include <zconf.h>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "DrmInfo.h"
#include "GbmInfo.h"

using namespace std;

void ShowMessage(const string & message)
{
    cerr << message << endl;
}

void ShowMessageAndExit(const string & message)
{
    ShowMessage(message);
    exit(EXIT_FAILURE);
}

void ShowErrorAndExit(const string & message)
{
    int errorCode = errno;
    cerr << message << ": " << strerror(errorCode) << "(" << errorCode << ")" << endl;
    exit(EXIT_FAILURE);
}

//void ShowFrameBuffer(const Drm::FrameBufferInfo * frameBuffer, int indent)
//{
//    cout << string(indent, ' ') << "fb_id:             " << frameBuffer->_info.fb_id << endl;
//    cout << string(indent, ' ') << "size:              " << frameBuffer->_info.width << "x" << frameBuffer->_info.height << endl;
//    cout << string(indent, ' ') << "pitch:             " << frameBuffer->_info.pitch << endl;
//    cout << string(indent, ' ') << "depth:             " << frameBuffer->_info.depth << endl;
//    cout << string(indent, ' ') << "bpp:               " << frameBuffer->_info.bpp << endl;
//    cout << string(indent, ' ') << "handle:            " << frameBuffer->_info.handle << endl;
//}
//
//void ShowCrtc(const Drm::CRTCInfo * crtc, int indent)
//{
//    cout << string(indent, ' ') << "crtc_id:           " << crtc->_info.crtc_id << endl;
//    cout << string(indent, ' ') << "buffer_id:         " << crtc->_info.buffer_id << endl;
//    cout << string(indent, ' ') << "origin:            " << crtc->_info.x << "," << crtc->_info.y << endl;
//    cout << string(indent, ' ') << "size:              " << crtc->_info.width << "x" << crtc->_info.height << endl;
//    cout << string(indent, ' ') << "mode_valid:        " << crtc->_info.mode_valid << endl;
//    cout << string(indent, ' ') << "mode:              " << crtc->_info.mode.name << ": " << crtc->_info.mode.hdisplay << "x" << crtc->_info.mode.vdisplay << "@" << crtc->_info.mode.vrefresh << endl;
//    cout << string(indent, ' ') << "gamma_size:        " << crtc->_info.gamma_size << endl;
//}
//
//void ShowEncoder(const Drm::EncoderInfo * encoder, int indent)
//{
//    cout << string(indent, ' ') << "encoder_id:        " << encoder->GetID() << endl;
//    cout << string(indent, ' ') << "encoder_type:      " << encoder->GetType() << endl;
//    cout << string(indent, ' ') << "possible_crtcs:    " << encoder->GetNumPossibleCrtcs() << endl;
//    cout << string(indent, ' ') << "possible_clones:   " << encoder->GetNumPossibleClones() << endl;
//
//    if (encoder->GetCrtcID() != 0)
//    {
//        cout << string(indent, ' ') << "crtc:              " << encoder->GetCrtcID() << endl;
//        ShowCrtc(encoder->GetCrtc(), indent + 2);
//    }
//
//}
//
//void ShowVideoMode(const Drm::VideoMode & videoMode, int indent)
//{
//    cout << string(indent, ' ') << "name:     " << videoMode.info.name << endl;
//    cout << string(indent, ' ') << "type:     " << videoMode.info.type << endl;
//    cout << string(indent, ' ') << "flags:    " << hex << setw(8) << setfill('0') << videoMode.info.flags << dec << endl;
//    cout << string(indent, ' ') << "width:    " << videoMode.info.hdisplay << endl;
//    cout << string(indent, ' ') << "height:   " << videoMode.info.vdisplay << endl;
//    cout << string(indent, ' ') << "clock:    " << videoMode.info.clock << endl;
//    cout << string(indent, ' ') << "vrefresh: " << videoMode.info.vrefresh << endl;
//}
//
//void ShowProperty(const Drm::Property & property, int indent)
//{
//    cout << string(indent, ' ') << "id:       " << property.id << endl;
//    cout << string(indent, ' ') << "value:    " << property.value << endl;
//}
//
//void ShowConnector(const Drm::ConnectorInfo * connector, int indent)
//{
//    cout << string(indent, ' ') << "connector_id:      " << connector->GetID() << endl;
//    cout << string(indent, ' ') << "connector_type:    " << connector->GetType() << endl;
//    cout << string(indent, ' ') << "connector_type_id: " << connector->GetTypeID() << endl;
//    cout << string(indent, ' ') << "connected:         " << connector->GetConnectionState() << endl;
//    cout << string(indent, ' ') << "width/height (mm): " << connector->GetGeometryMM().width << "/" << connector->GetGeometryMM().height << endl;
//    cout << string(indent, ' ') << "subpixel:          " << connector->GetSubPixelMode() << endl;
//    cout << string(indent, ' ') << "encoder_id:        " << connector->GetConnectedEncoderID() << endl;
//    cout << string(indent, ' ') << "#modes:            " << connector->GetNumVideoModes() << endl;
//    for (size_t k = 0; k < connector->GetNumVideoModes(); ++k)
//    {
//        auto const & videoMode = connector->GetVideoMode(k);
//
//        cout << string(indent + 2, ' ') << "mode[" << k << "]" << endl;
//        ShowVideoMode(videoMode, indent + 4);
//    }
//
//    cout << string(indent, ' ') << "#properties:       " << connector->GetNumProperties() << endl;
//    for (int k = 0; k < connector->GetNumProperties(); ++k)
//    {
//        cout << string(indent + 2, ' ') << "property[" << k << "]:" << endl;
//        ShowProperty(connector->GetPropertyByIndex(k), indent + 4);
//    }
//
//    cout << string(indent, ' ') << "#encoders:         " << connector->GetNumEncoders() << endl;
//    for (int k = 0; k < connector->GetNumEncoders(); ++k)
//    {
//        cout << string(indent + 2, ' ') << "encoder[" << k << "] = " << connector->GetEncoderByIndex(k)->GetID() << endl;
//        ShowEncoder(connector->GetEncoderByIndex(k), indent + 4);
//    }
//}
void CheckFormatSupport(const Gbm::Device & gbmDevice, Gbm::SampleFormat format, int indent)
{
    cout << string(indent, ' ') << format << ": " << (gbmDevice.SupportsFormat(format, 0) ? "Y" : "N") << endl;
}

void CheckFormatSupport(const Gbm::Device & gbmDevice, int indent)
{
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::C8, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::R8, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::GR88, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGB332, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGR233, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::XRGB4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::XBGR4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGBX4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGRX4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::ARGB4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::ABGR4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGBA4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGRA4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::XRGB1555, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::XBGR1555, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGBX5551, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGRX5551, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::ARGB1555, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::ABGR1555, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGBA5551, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGRA5551, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGB565, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGR565, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGB888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGR888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::XRGB8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::XBGR8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGBX8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGRX8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::ARGB8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::ABGR8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGBA8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGRA8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::XRGB2101010, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::XBGR2101010, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGBX1010102, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGRX1010102, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::ARGB2101010, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::ABGR2101010, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::RGBA1010102, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::BGRA1010102, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YUYV, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YVYU, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::UYVY, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::VYUY, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::AYUV, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::NV12, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::NV21, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::NV16, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::NV61, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YUV410, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YVU410, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YUV411, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YVU411, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YUV420, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YVU420, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YUV422, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YVU422, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YUV444, indent);
    CheckFormatSupport(gbmDevice, Gbm::SampleFormat::YVU444, indent);
}

int main(int argc, char * argv[])
{
    size_t numCards = Drm::CardsInfo::GetNumCards();
    cout << "Found " << numCards << " video card" << (numCards > 1 ? "s" : "") << endl;

    for (size_t i = 0; i < numCards; ++i)
    {
        Drm::CardInfo cardInfo = Drm::CardInfo::OpenCard(i);
        if (!cardInfo.IsOpen())
        {
            // Probably permissions error
            ostringstream stream;
            stream << "Couldn't open video card " << i << ": " << Drm::CardsInfo::GetDeviceName(i);
            ShowErrorAndExit(stream.str());
        }
        Gbm::Device gbmDevice(cardInfo.GetFD());
        if (!gbmDevice.IsValid())
        {
            ostringstream stream;
            stream << "Couldn't create GBM device " << i;
            ShowErrorAndExit(stream.str());
        }

        cout << "backend name:        " << gbmDevice.GetBackendName() << endl;
        cout << "Checking for format support:" << endl;
        CheckFormatSupport(gbmDevice, 2);
    }

    return 0;
}
