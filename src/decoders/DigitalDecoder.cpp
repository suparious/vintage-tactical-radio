#include "DigitalDecoder.h"

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

DigitalDecoder::DigitalDecoder(DecoderType type, QObject* parent)
    : QObject(parent)
    , type_(type)
    , active_(false)
    , state_(DecoderState::IDLE)
    , sampleRate_(48000) {
}

void DigitalDecoder::setState(DecoderState state) {
    if (state_ != state) {
        state_ = state;
        emit stateChanged(state);
        
#ifdef HAS_SPDLOG
        const char* stateStr = "";
        switch (state) {
            case DecoderState::IDLE: stateStr = "IDLE"; break;
            case DecoderState::SEARCHING: stateStr = "SEARCHING"; break;
            case DecoderState::SYNCING: stateStr = "SYNCING"; break;
            case DecoderState::DECODING: stateStr = "DECODING"; break;
            case DecoderState::ERROR: stateStr = "ERROR"; break;
        }
        spdlog::debug("Decoder state changed to: {}", stateStr);
#endif
    }
}

void DigitalDecoder::emitData(const QVariantMap& data) {
    emit dataDecoded(data);
}

void DigitalDecoder::emitError(const QString& error) {
    emit errorOccurred(error);
#ifdef HAS_SPDLOG
    spdlog::error("Decoder error: {}", error.toStdString());
#endif
}
