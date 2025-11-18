

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

                Connections {
                    target: start_button
                    function onClicked() {
                        console.log("clicked")
                    }
                }
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

                Connections {
                    target: pause_button
                    function onClicked() {
                        console.log("clicked")
                    }
                }
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

                Connections {
                    target: stop_button
                    function onClicked() {
                        console.log("clicked")
                    }
                }
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

                Connections {
                    target: rotate_right
                    function onClicked() {
                        console.log("clicked")
                    }
                }
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

                Connections {
                    target: rotate_left
                    function onClicked() {
                        console.log("clicked")
                    }
                }
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

                Connections {
                    target: forward_button
                    function onClicked() {
                        console.log("clicked")
                    }
                }
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

                Connections {
                    target: back_button
                    function onClicked() {
                        console.log("clicked")
                    }
                }
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

                Connections {
                    target: right_button
                    function onClicked() {
                        console.log("clicked")
                    }
                }
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

                Connections {
                    target: left_button
                    function onClicked() {
                        console.log("clicked")
                    }
                }
            }

            SpinBox {
                id: spinBox
                x: 48
                y: 425
            }

            SpinBox {
                id: spinBox1
                x: 48
                y: 486
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

            Rectangle {
                id: rectangle1
                x: 417
                y: 416
                width: 200
                height: 200
                color: "#ffffff"
            }

            Text {
                id: text1
                x: 418
                y: 399
                text: qsTr("Terminal:")
                font.pixelSize: 12
            }

            Text {
                id: text2
                x: 38
                y: 389
                width: 262
                height: 17
                text: qsTr("Zmiana położenia o wpisaną wartość:")
                font.pixelSize: 12
                font.bold: false
                font.family: "Verdana"
            }

            Text {
                id: text3
                x: 50
                y: 409
                text: qsTr("X:")
                font.pixelSize: 12
            }

            Text {
                id: text4
                x: 50
                y: 470
                text: qsTr("Y:")
                font.pixelSize: 12
            }

            Button {
                id: button
                x: 48
                y: 535
                width: 120
                height: 32
                text: qsTr("Wyślij wartość:")
            }

            Text {
                id: text5
                x: 48
                y: 615
                width: 262
                height: 17
                text: qsTr("Stan aktualnego zlecenia:")
                font.pixelSize: 12
                font.family: "Verdana"
                font.bold: false
            }

            Text {
                id: text6
                x: 424
                y: 421
                width: 164
                height: 16
                text: qsTr("linijka")
                font.pixelSize: 12
                font.family: "Courier"
            }

            Text {
                id: text7
                x: 424
                y: 441
                width: 164
                height: 16
                text: qsTr("linijka")
                font.pixelSize: 12
                font.family: "Courier"
            }

            Text {
                id: text8
                x: 424
                y: 463
                width: 164
                height: 16
                text: qsTr("linijka")
                font.pixelSize: 12
                font.family: "Courier"
            }

            Text {
                id: text9
                x: 424
                y: 486
                width: 164
                height: 16
                text: qsTr("linijka")
                font.pixelSize: 12
                font.family: "Courier"
            }

            Text {
                id: text10
                x: 424
                y: 508
                width: 164
                height: 16
                text: qsTr("linijka")
                font.pixelSize: 12
                font.family: "Courier"
            }

            Text {
                id: text11
                x: 424
                y: 530
                width: 164
                height: 16
                text: qsTr("linijka")
                font.pixelSize: 12
                font.family: "Courier"
            }

            Text {
                id: text12
                x: 424
                y: 552
                width: 164
                height: 16
                text: qsTr("linijka")
                font.pixelSize: 12
                font.family: "Courier"
            }

            Text {
                id: text13
                x: 424
                y: 574
                width: 164
                height: 16
                text: qsTr("linijka")
                font.pixelSize: 12
                font.family: "Courier"
            }

            Text {
                id: text14
                x: 424
                y: 596
                width: 164
                height: 16
                text: qsTr("linijka")
                font.pixelSize: 12
                font.family: "Courier"
            }
        }
    }
}
