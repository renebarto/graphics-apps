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
void CheckFormatSupport(const Gbm::Device & gbmDevice, Gbm::Device::SampleFormat format, int indent)
{
    cout << string(indent, ' ') << format << ": " << (gbmDevice.SupportsFormat(format, 0) ? "Y" : "N") << endl;
}

void CheckFormatSupport(const Gbm::Device & gbmDevice, int indent)
{
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::C8, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::R8, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::GR88, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGB332, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGR233, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::XRGB4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::XBGR4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGBX4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGRX4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::ARGB4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::ABGR4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGBA4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGRA4444, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::XRGB1555, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::XBGR1555, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGBX5551, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGRX5551, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::ARGB1555, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::ABGR1555, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGBA5551, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGRA5551, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGB565, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGR565, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGB888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGR888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::XRGB8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::XBGR8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGBX8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGRX8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::ARGB8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::ABGR8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGBA8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGRA8888, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::XRGB2101010, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::XBGR2101010, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGBX1010102, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGRX1010102, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::ARGB2101010, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::ABGR2101010, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::RGBA1010102, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::BGRA1010102, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YUYV, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YVYU, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::UYVY, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::VYUY, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::AYUV, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::NV12, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::NV21, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::NV16, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::NV61, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YUV410, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YVU410, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YUV411, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YVU411, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YUV420, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YVU420, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YUV422, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YVU422, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YUV444, indent);
    CheckFormatSupport(gbmDevice, Gbm::Device::SampleFormat::YVU444, indent);
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
//        cout << "#crtc:           " << cardInfo.GetNumCRTCs() << endl;
//        cout << "#connectors:     " << cardInfo.GetNumConnectors() << endl;
//        cout << "#encoders:       " << cardInfo.GetNumEncoders() << endl;
//        cout << "#width min/max:  " << cardInfo.GetMinSizePixels().width << "/" << cardInfo.GetMinSizePixels().height << endl;
//        cout << "#height min/max: " << cardInfo.GetMaxSizePixels().width << "/" << cardInfo.GetMaxSizePixels().height << endl;
//
//        for (size_t j = 0; j < cardInfo.GetNumFrameBuffers(); ++j)
//        {
//            auto frameBuffer = cardInfo.GetFrameBufferByIndex(j);
//            if (!frameBuffer)
//            {
//                ostringstream stream;
//                stream << "Cannot find frameBuffer " << j;
//                ShowMessage(stream.str());
//                continue;
//            }
//            cout << "frameBuffer[" << j << "]" << endl;
//            ShowFrameBuffer(frameBuffer, 2);
//        }
//        for (size_t j = 0; j < cardInfo.GetNumCRTCs(); ++j)
//        {
//            auto crtc = cardInfo.GetCRTCByIndex(j);
//            if (!crtc)
//            {
//                ostringstream stream;
//                stream << "Cannot find crtc " << j;
//                ShowMessage(stream.str());
//                continue;
//            }
//            cout << "crtc[" << j << "]" << endl;
//            ShowCrtc(crtc, 2);
//        }
//        for (size_t j = 0; j < cardInfo.GetNumConnectors(); ++j)
//        {
//            auto connector = cardInfo.GetConnectorByIndex(j);
//
//            if (!connector)
//            {
//                ostringstream stream;
//                stream << "Cannot find connector " << j;
//                ShowMessage(stream.str());
//                continue;
//            }
//            cout << "connector[" << j << "]" << endl;
//            ShowConnector(connector, 2);
//        }
//
//        for (size_t j = 0; j < cardInfo.GetNumEncoders(); ++j)
//        {
//            auto encoder = cardInfo.GetEncoderByIndex(j);
//            if (!encoder)
//            {
//                ostringstream stream;
//                stream << "Cannot find encoder " << j;
//                ShowMessage(stream.str());
//                continue;
//            }
//            cout << "encoder[" << j << "]" << endl;
//            ShowEncoder(encoder, 2);
//        }
//        auto activeScreens = cardInfo.GetActiveConnectors();
//        cout << "Active screens" << endl;
//        for (auto screen : activeScreens)
//        {
//            Drm::Rect rc = screen->GetRect();
//            cout << "  Type: " << screen->GetType() << " (" << rc.x << "," << rc.y << ","
//                 << rc.width << "," << rc.height << ")" << endl;
//        }
//        cout << "Primary screen" << endl;
//        auto primaryScreen = cardInfo.GetPrimaryConnector();
//        Drm::Rect rc = primaryScreen->GetRect();
//        cout << "  Type: " << primaryScreen->GetType() << " (" << rc.x << "," << rc.y << ","
//             << rc.width << "," << rc.height << ")" << endl;
    }

    return 0;
}
