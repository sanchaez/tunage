import QtQuick 2.11
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2

ToolBar {
    id: buttonToolbar

    property bool playing: false
    property bool stopped: true
    property int lastVolumePos: 100
    property alias mediaDuration: progressBar.to

    property alias mediaArtistName: artistNameLabel.text
    property alias mediaName: mediaNameLabel.text
    property alias mediaVolume: volumeSlider.value
    property alias mediaMuted: muteButton.checked
    property alias mediaCoverUrl: albumCover.albumArt

    property int newMediaPosition: 0
    property int lastMediaPosition: 0
    property int mediaPosition: 0
    property bool scrobbling: false

    signal stop
    signal playlist
    signal back
    signal forward
    signal menu
    signal scrobble(int pos)
    signal mute(bool isMute)
    signal playback(bool isPlay)
    signal positionUpdatingChanged(bool isUpdating)

    // connections to signals
    Connections {
        target: progressBar
        onPressedChanged: {
            if(!progressBar.pressed) {
                newMediaPosition = progressBar.value
                scrobbling = false
            } else {
                lastMediaPosition = progressBar.value
                scrobbling = true
            }
        }
    }

    Material.elevation: 10

    RowLayout {
        anchors.fill: parent
        spacing: 10

        // album art
        Rectangle {
            id: albumCover
            Layout.preferredWidth: parent.height
            Layout.preferredHeight: parent.height
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            color: Material.accent

            property string albumArt: ""

            Image {
                anchors.centerIn: parent
                anchors.fill: parent
                mipmap: true

                fillMode: Image.PreserveAspectFit

                source: albumCover.albumArt != ""
                        ? albumCover.albumArt
                        : "qrc:/external/googleicons/av/ic_album_48px.svg"
            }
        }

        ColumnLayout {
            Layout.fillWidth: true

            // progress bar
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 20
                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                Slider {
                    id: progressBar
                    from: 0
                    to: 1
                    topPadding: 0
                    leftPadding: 0
                    bottomPadding: 0
                    Layout.preferredHeight: 14
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    Layout.fillWidth: true

                    value: scrobbling ? lastMediaPosition : mediaPosition

                    onHoveredChanged: {
                        state = hovered || pressed ? "active" : "inactive"
                    }

                    onPressedChanged: {
                        state = hovered || pressed ? "active" : "inactive"
                    }

                    states: [
                        State {
                            name: "inactive"
                            PropertyChanges { target: progressBarBackground; opacity: 0.0}
                        },
                        State {
                            name: "active"
                            PropertyChanges { target: progressBarBackground; opacity: 1.0}
                        }
                    ]

                    transitions: [
                        Transition {
                            PropertyAnimation {
                                target: progressBarBackground
                                properties: "opacity"

                                duration: 300
                                easing {type: Easing.InOutQuad; overshoot: 500}}
                        }
                    ]

                    background: Item {
                        Rectangle {
                            id: progressBarBackground
                            x: progressBar.leftPadding
                            y: progressBar.topPadding + progressBar.availableHeight / 2 - height / 2
                            width: progressBar.availableWidth
                            height: progressBar.availableHeight / 3
                            color: Material.shade(Material.background, Material.Shade600)
                            opacity: 0.0

                            Text {
                                id: progressText
                                anchors.right: parent.right
                                y: progressBar.availableHeight
                                text: getTimeFromMSec(mediaPosition) + "/" + getTimeFromMSec(mediaDuration)
                                font.pixelSize: 10
                            }
                        }

                        Rectangle {
                            x: progressBarBackground.x
                            y: progressBarBackground.y
                            width: progressBar.visualPosition * progressBarBackground.width
                            height: progressBarBackground.height
                            color: Material.accent
                        }
                    }
                }
            }


            RowLayout {
                Layout.fillWidth: true

                // media name
                Column {
                    Layout.preferredWidth: 100
                    Layout.fillWidth: true
                    Text {
                        id: mediaNameLabel
                        font.pixelSize: 16
                        width: parent.width
                        clip: true
                    }

                    Text {
                        id: artistNameLabel
                        font.pixelSize: 12
                        width: parent.width
                        clip: true
                    }
                }

                Item {
                    Layout.preferredWidth: 10
                }

                // volume control
                Row {
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: 120

                    ToolButton {
                        id: muteButton
                        checkable: true

                        icon.source: checked
                                     ? "volume_off_24px"
                                     : (volumeSlider.value == 0)
                                       ? "volume_mute_24px"
                                       : (volumeSlider.value < (volumeSlider.to / 2)
                                        ? "volume_down_24px"
                                        : "volume_up_24px")
                        icon.width: 24
                        icon.height: 24

                        onToggled: {
                            if (checked) {
                                lastVolumePos = volumeSlider.value
                                volumeSlider.value = 0
                            } else {
                                volumeSlider.value = lastVolumePos
                            }
                        }
                    }

                    Slider {
                        id: volumeSlider
                        from: 0
                        to: 100
                        width: parent.width - muteButton.width
                        stepSize: 1

                        onMoved: {
                            if (muteButton.checked) {
                                mediaVolume = volumeSlider.value
                                muteButton.checked = false
                            }
                        }

                        Component.onCompleted: value = to
                    }
                }

                // main player controls
                RowLayout {
                    Layout.alignment: Qt.AlignCenter
                    ToolButton {
                        id: prevButton
                        icon.source: "skip_previous_24px"
                        icon.width: 24
                        icon.height: 24

                        onClicked: back()
                    }

                    RoundButton {
                        id: playButton

                        icon.source: playing ? "pause_48px"
                                             : "play_arrow_48px"
                        icon.width: 24
                        icon.height: 24

                        onClicked: playback(!playing)
                        highlighted: true
                        Material.elevation: 0
                    }

                    ToolButton {
                        id: nextButton

                        icon.source: "skip_next_24px"
                        icon.width: 24
                        icon.height: 24

                        onClicked: forward()
                    }
                }

                Item { Layout.fillWidth: true }

                ToolButton {
                    id: menuButton
                    Layout.alignment: Qt.AlignRight
                    icon.source: "qrc:/external/googleicons/navigation/ic_menu_24px.svg"
                    icon.width: 24
                    icon.height: 24
                    onClicked: menu()
                }

            }
        }
    }
}
