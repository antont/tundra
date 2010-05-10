// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_MumbleVoipModule_Connection_h
#define incl_MumbleVoipModule_Connection_h

#include <QObject>
#include <QList>
#include <QMutex>
#include <QMap>
#include <QPair>
#include "Core.h"
#include "CoreTypes.h"
#include "ServerInfo.h"
#include "stdint.h"
#include "MumbleDefines.h"

class QNetworkReply;
class QNetworkAccessManager;
namespace MumbleClient
{
    class MumbleClient;
    class Channel;
    class User;
}
struct CELTMode;
struct CELTEncoder;
struct CELTDecoder;

namespace MumbleVoip
{
    class Channel;
    class User;
    class PCMAudioFrame;

    typedef QPair<User*, PCMAudioFrame*> AudioPacket;

    //! Connection to a single mumble server.
    //!
    //! Do not use this class directly. Only ConnectionManager class is supposed
    //! to use this class.
    //!
    //! This is basically a wrapper over Client class of mumbleclient library.
    //  Mumbleclient library has a main loop whitch calls callback functions in this class
    //! so thread safaty have to be dealed within this class.
    //! 
    //! Connections has Channel and User objects.
    class Connection : public QObject
    {
        Q_OBJECT
    public:
        enum State { STATE_INITIALIZING, STATE_OPEN, STATE_CLOSED, STATE_ERROR };

        //! Default constructor
        Connection(ServerInfo &info);

        //! Default deconstructor
        virtual ~Connection();

        //! Closes connection to Mumble server
        virtual void Close();

        //! Joins to given channel if channels exist
        //! If authtorization is not completed yet the join request is queuesd
        //! and executed again after successfullu authorization
        //! @todo HANDLE REJOIN
        virtual void Join(QString channel);

        //! Joins to given channel
        //! @todo HANDLE REJOIN
        virtual void Join(const Channel* channel);

        //! @return first <user,audio frame> pair from playback queue
        //!         return <0,0> if playback queue is empty
        //! The caller must delete audio frame object after usage
        virtual AudioPacket GetAudioPacket();

        //! Encode and send given frame to Mumble server
        //! Frame object is NOT deleted by this method 
        virtual void SendAudioFrame(PCMAudioFrame* frame, Vector3df users_position);

        //! @return list of channels available
        //! @todo CONSIDER TO USE boost::weak_ptr HERE
        virtual QList<Channel*> ChannelList();

        //! @return channel by id. Return 0 if channel cannot be found
        virtual MumbleVoip::Channel* ChannelById(int id);

        //! @param name The full name of the channel e.g "Root/mychannel"
        //! @return channel by name. Return 0 if channel cannot be found
        virtual MumbleVoip::Channel* ChannelByName(QString name);

        //! Set audio sending true/false 
        //! @param send true if audio want to be sent to mumble server, otherwise false
        virtual void SendAudio(bool send);

        //! @return true if connection is sending audio, return false otherwise
        virtual bool SendingAudio() const;

        //! Set audio sending true/false 
        //! @param receive true if received audio packets should be handled, false if 
        //!                received audio packets should be ignoered
        virtual void ReceiveAudio(bool receive);

        //! \param quality [0.0 .. 1.0] where:
        //!        0.0 means lowest bitrate and worst quality
        //!        1.0 means highest bitrate and best quality.
        virtual void SetEncodingQuality(double quality);

        //! Set position sending on/off
        //! @param send true if position information should be sent with audio data, false otherwise
        //! @see UpdatePosition to set current position to be sent.
        virtual void SendPosition(bool send) { send_position_ = send; }

        //! @return true if position is sent to server with audio data, otherwise false
        virtual bool IsSendingPosition() const { return send_position_; }

        //! @return current state of the connection
        //! @see GetState() to get reason for the state
        virtual State GetState() const;

        //! @return textual description for the reason for current state
        //! @see GetState() to get state
        virtual QString GetReason() const;

    public slots:

        void SetAuthenticated();

        //! @param text Text message received from Mumble server
        void HandleIncomingTextMessage(QString text);

        //! @param length the length of packet in bytes
        //! @param the packet data
        void HandleIncomingRawUdpTunnelPacket(int32_t length, void* buffer);

//        void OnRelayTunnel(std::string &s);

        //! Create a new Channel object if channels doesn't already exist
        void AddChannel(const MumbleClient::Channel& channel);

        //! Remove channel if it already exist
        void RemoveChannel(const MumbleClient::Channel& channel);

        //! Add user if it doesn't already exit
        void Adduser(const MumbleClient::User& user);

        //! Remove user from user list if it exist
        void RemoveUser(const MumbleClient::User& user);

    private:
        static const int AUDIO_QUALITY_MAX_ = 90000; 
        static const int AUDIO_QUALITY_MIN_ = 32000; 
        static const int ENCODE_BUFFER_SIZE_ = 4000;

        void InitializeCELT();
        void UninitializeCELT();
        CELTDecoder* CreateCELTDecoder();
        int AudioQuality();
        void HandleIncomingCELTFrame(int session, unsigned char* data, int size);
        bool CheckState(QList<State> allowed_states); // testing

        State state_;
        QString reason_;
        MumbleClient::MumbleClient* client_;
        QString join_request_; // queued request to join a channel @todo IMPLEMENT BETTER
        QList<PCMAudioFrame*> encode_queue_;
        QList<Channel*> channels_; // @todo Use shared ptr
        QMap<int, User*> users_; // maps: session id <-> User object

        CELTMode* celt_mode_;
        CELTEncoder* celt_encoder_;
        CELTDecoder* celt_decoder_;

        bool authenticated_;
        bool sending_audio_;
        bool receiving_audio_;
        double encoding_quality_;
        int frame_sequence_;
        bool send_position_;
        unsigned char encode_buffer_[ENCODE_BUFFER_SIZE_];
        
        QMutex mutex_channels_;
        QMutex mutex_authentication_;
        QMutex mutex_encode_audio_;
        QMutex mutex_encoding_quality_;
        QMutex mutex_raw_udp_tunnel_;
        QMutex mutex_users_;
        QMutex mutex_state_;
        
    signals:
        void StateChanged(State state);
        void TextMessageReceived(QString &text); 
        void AudioDataAvailable(short* data, int size);
        //! emited when user left from server
        void UserLeft(User* user);
        //! emited when user join to server
        void UserJoined(User* user);
        void ChannelAdded(Channel* channel); 
        void ChannelRemoved(Channel* channel);
    };

} // namespace MumbleVoip

#endif // incl_MumbleVoipModule_Connection_h
