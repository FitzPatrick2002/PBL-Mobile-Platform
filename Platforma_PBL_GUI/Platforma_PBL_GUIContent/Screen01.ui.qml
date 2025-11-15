

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import Platforma_PBL_GUI

Rectangle {
    id: rectangle1
    width: Constants.width
    height: Constants.height
    color: "#071622"

    Image {
        id: image
        x: 0
        y: 0
        width: 1200
        height: 800
        source: "images/WelcomeWindow.png"
        fillMode: Image.PreserveAspectFit

        Button {
            id: about_us_button
            x: 81
            y: 628
            width: 201
            height: 44
            text: qsTr("About Us")
            hoverEnabled: false
            font.family: "Arial"
            font.bold: true
            font.pointSize: 20
            flat: true

            Connections {
                target: about_us_button
                function onClicked() {
                    aboutUsOpening.open_webbrowser()
                }
            }
        }

        Button {
            id: hi_dora_button
            x: 317
            y: 628
            width: 200
            height: 44
            text: qsTr("HI DORA")
            hoverEnabled: false
            clip: false
            autoExclusive: false
            font.bold: true
            font.pointSize: 20
            font.family: "Arial"
            flat: true

            Connections {
                target: hi_dora_button
                function onClicked() {
                    navigation.openScreen("screen2")
                }
            }
        }

        Button {
            id: quit_button
            x: 918
            y: 698
            width: 202
            height: 44
            text: qsTr("QUIT")
            icon.color: "#c82424"
            wheelEnabled: false
            hoverEnabled: false
            font.kerning: false
            font.preferShaping: false
            layer.mipmap: false
            layer.smooth: false
            focus: false
            antialiasing: false
            activeFocusOnTab: false
            enabled: true
            smooth: false
            layer.enabled: false
            font.bold: true
            font.pointSize: 16
            font.family: "Arial"
            flat: true

            Connections {
                target: quit_button
                function onClicked() {
                    Qt.quit()
                }
            }
        }
    }
}
