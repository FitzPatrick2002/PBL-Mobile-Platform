

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtCharts


Item {
    id: root
    width: 1200
    height: 800

    Rectangle {
        id: rectangle
        x: 0
        y: 0
        width: 1200
        height: 800
        color: "#ffffff"

        Image {
            id: controlWindow
            x: 0
            y: 0
            source: "images/ControlWindow (1).png"
            fillMode: Image.PreserveAspectFit

            Button {
                id: start_button
                x: 564
                y: 114
                width: 114
                height: 45
                text: qsTr("Start")
                hoverEnabled: false
                display: AbstractButton.IconOnly
                flat: true
            }

            Button {
                id: pause_button
                x: 565
                y: 170
                width: 114
                height: 45
                text: qsTr("Start")
                hoverEnabled: false
                flat: true
                display: AbstractButton.IconOnly
            }

            Button {
                id: stop_button
                x: 512
                y: 235
                width: 218
                height: 58
                text: qsTr("Start")
                hoverEnabled: false
                flat: true
                display: AbstractButton.IconOnly
            }

            Button {
                id: rotate_right
                x: 291
                y: 66
                width: 194
                height: 122
                text: qsTr("Start")
                hoverEnabled: false
                flat: true
                display: AbstractButton.IconOnly
            }

            Button {
                id: rotate_left
                x: 289
                y: 199
                width: 194
                height: 122
                text: qsTr("Start")
                hoverEnabled: false
                flat: true
                display: AbstractButton.IconOnly
            }

            Button {
                id: forward_button
                x: 121
                y: 89
                width: 82
                height: 57
                text: qsTr("Start")
                hoverEnabled: false
                flat: true
                display: AbstractButton.IconOnly
            }

            Button {
                id: back_button
                x: 120
                y: 256
                width: 82
                height: 57
                text: qsTr("Start")
                hoverEnabled: false
                flat: true
                display: AbstractButton.IconOnly
            }

            Button {
                id: right_button
                x: 217
                y: 160
                width: 58
                height: 83
                text: qsTr("Start")
                hoverEnabled: false
                flat: true
                display: AbstractButton.IconOnly
            }

            Button {
                id: left_button
                x: 48
                y: 160
                width: 58
                height: 83
                text: qsTr("Start")
                hoverEnabled: false
                flat: true
                display: AbstractButton.IconOnly
            }

            SpinBox {
                id: spinBox
                x: 66
                y: 443
            }

            SpinBox {
                id: spinBox1
                x: 66
                y: 500
            }

            ProgressBar {
                id: progressBar
                x: 48
                y: 632
                width: 556
                height: 20
                value: 0.5

                Connections {
                    target: progressBar
                    function onValueChanged() {
                        root.state = ""
                    }
                }
            }
        }
    }
}
