import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import VisRenderOpenGL 1.0

ApplicationWindow {
    id: window
    visible: true
    width: 800
    height: 480
    minimumWidth: 600
    minimumHeight: 400
    title: qsTr("Tunage")
    visibility: "Windowed"

    property bool fullscreen: false

    function getTimeFromMSec(msec) {
         if (msec <= 0 || msec === undefined) {
             return "0:00"

         } else {
             var sec = "" + Math.floor(msec / 1000) % 60
             if (sec.length == 1)
                 sec = "0" + sec

             var min = "" + Math.floor(msec / 60000) % 60
             if (min.length == 1)
                 min = "0" + min

             var hour = Math.floor(msec / 3600000)

             if (hour < 1) {
                return min + ":" + sec
             } else {
                return hour + ":" + min + ":" + sec
             }
         }
     }

    Connections {
        target: appController
        onError: {
            messageDialog.title = "Sound Engine Error!"
            messageDialog.text = what
            messageDialog.icon = StandardIcon.Warning
            messageDialog.open()
        }
        onLoadingFileStarted: fileLoadStartHelper.restart()
        onLoadingFileFinished: fileLoadFinishHelper.restart()
    }

    // non-blocking connections
    Timer {
        id: fileLoadStartHelper
        interval: 1
        onTriggered: filePopup.open()
    }

    Timer {
        id: fileLoadFinishHelper
        interval: 1
        onTriggered: filePopup.close()
    }

    Binding {
        target: appController
        property: "volume"
        value: playerButtons.mediaVolume
    }

    MessageDialog {
        id: messageDialog
        Component.onCompleted: visible = false
    }

    Popup {
        id: popup
        property string text
        property string title

        parent: window.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: 100
        height: 100

        ColumnLayout {
            anchors.fill: parent
            Text {
                text: popup.title
            }
            Text {
                Layout.fillHeight: true
                text: popup.text
            }
            Button {
                onClicked: popup.close()
            }
        }
    }

    Popup {
        id: filePopup

        parent: window.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: 100
        height: 30

        Rectangle {
            anchors.fill: parent

            ProgressBar {
                anchors.fill: parent
                indeterminate: true
            }
        }
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: qsTr("Select new shader...")
            onClicked: shaderFileDialog.open()
        }

        MenuItem {
            text: qsTr("Fullscreen")
            onClicked: {
                window.visibility = fullscreen ? "Windowed" : "FullScreen"
                fullscreen = !fullscreen
            }
        }
    }

    Visualisation {
        id: visualizer
        width: window.width
        height: window.height
        waveform: appController.waveform
        spectrum: appController.spectrum

        MouseArea {
            anchors.fill: parent
            property bool wasPlaylistOpen: false
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: {
                if (mouse.button === Qt.RightButton) {
                    contextMenu.popup()
                } else {
                    if(buttonDrawer.visible && playlistDrawer.visible) {
                        wasPlaylistOpen = true
                    } else if(buttonDrawer.visible && !playlistDrawer.visible){
                        wasPlaylistOpen = false
                    }

                    playlistDrawer.visible = !playlistDrawer.visible && wasPlaylistOpen
                    buttonDrawer.visible = !buttonDrawer.visible
                }
            }
        }
    }

    Drawer {
        id: playlistDrawer
        width:  window.width < 400 ? 0.4 * window.width : 300
        height: window.height - buttonDrawer.availableHeight
        modal: false
        interactive: false
        position: 0
        edge: Qt.RightEdge
        ColumnLayout {
            anchors.fill: parent
            ToolBar {
                id: toolbar

                Layout.fillWidth: true

                RowLayout {
                    anchors.fill: parent

                    Item {
                        Layout.fillWidth: true
                    }
                    ToolButton {
                        text: qsTr("Load")
                        onClicked: musicFileDialog.open()
                    }
                    ToolButton {
                        text: qsTr("Clear")
                        onClicked: appController.removeAllFiles()
                    }
                    ToolButton {
                        icon.source: "more_24px"
                        onClicked: contextMenu.popup()
                    }
                }
            }

            ListView {
                id: playlist
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 1

                clip: true

                Connections {
                    target: playlist.model
                    onCurrentIndexChanged: playlist.currentIndex = index
                }

                model: appController.playlist
                delegate: Item {
                    id: item
                    width: parent.width
                    height: 24

                    RowLayout {
                        anchors.fill: item
                        Text {
                            Layout.alignment: Qt.AlignRight
                            text: getTimeFromMSec(duration)
                            font.pixelSize: 12
                            padding: 5
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        Text {
                            text: name
                                  ? name + (artist ? " - " + artist : "noname")
                                  : fileName
                            padding: 3
                            font.pixelSize: 12
                        }
                        Item {
                            Layout.preferredWidth: 5
                        }
                    }
                    MouseArea {
                        anchors.fill: parent

                        acceptedButtons: Qt.LeftButton | Qt.RightButton;

                        onPressed: {
                            if (mouse.button === Qt.LeftButton) {
                                playlist.currentIndex = index
                                appController.setCurrentItem(index)
                            }
                        }

                        onReleased: {
                            if (mouse.button === Qt.RightButton) {
                                appController.removeItem(index)
                            }
                        }

                        onDoubleClicked: {
                            if (mouse.button === Qt.LeftButton) {
                                appController.setCurrentItem(index)
                                appController.play(true)
                            }
                        }
                    }
                }

                highlight: Rectangle {
                    color: appController.isPlaying ? Material.accent : Material.color(Material.Grey)
                }

                focus: true
            }
        }

        visible: fullscreen ? false : visible

        Component.onCompleted: visible = false
    }

    // setup shortcuts
    Shortcut {
        sequence: "Space"
        onActivated: !playerButtons.playing
                     ? appController.play()
                     : appController.pause()
    }

    Shortcut {
        sequence: StandardKey.Quit
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    FileDialog {
        id: shaderFileDialog
        title: qsTr("Please choose a .glsl file")
        folder: "/shaders"
        nameFilters: ["Shaders (*.glsl)"]
        selectMultiple: false
        onAccepted: {
            visualizer.currentShader = fileUrl
        }

        Component.onCompleted: visible = false
    }

    FileDialog {
        id: musicFileDialog
        title: qsTr("Please choose an audio file")
        folder: shortcuts.music
        nameFilters: ["Audio Files (*.mp3 *.wav *.flac *.ogg *.oga *.opus *.wv *.wc)", "All Files (*)"]
        selectMultiple: true
        onAccepted: {
            appController.addFiles(fileUrls)
        }
        Component.onCompleted: visible = false
    }

    Drawer {
        id: buttonDrawer
        height: 96
        width: parent.width
        edge: Qt.BottomEdge
        modal: false
        interactive: false
        position: 1

        ButtonRow {
            id: playerButtons
            anchors.fill: parent
            //mediaVolume: appController.volume

            // bind to SoundEngineObj
            mediaPosition: appController.position
            mediaDuration: appController.duration
            mediaName: appController.song
            mediaArtistName: appController.artist
            mediaCoverUrl: appController.coverUrl

            playing: appController.isPlaying

            onStoppedChanged: appController.stop()
            onNewMediaPositionChanged: appController.setPosition(newMediaPosition)
            onMediaMutedChanged: appController.setMuted(muted)
            //onPositionUpdatingChanged: appController.stopPositionUpdate = isUpdating
            onMenu: playlistDrawer.visible ? playlistDrawer.close() : playlistDrawer.open()
            onForward: appController.next()
            onBack: appController.previous()
            onPlayback: appController.play(isPlay)
        }

        Component.onCompleted: visible = true
    }
}
