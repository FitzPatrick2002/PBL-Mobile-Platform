

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
    width: Constants.width
    height: Constants.height
    color: "#071622"
    property alias stopButtonFlat: stopButton.flat

    Rectangle {
        id: rectangle
        x: 61
        y: 168
        width: 615
        height: 395
        color: "#ffffff"

        Button {
            id: button
            x: 131
            y: 56
            text: qsTr("Button")
        }

        SwitchDelegate {
            id: switchDelegate
            x: 90
            y: 277
            text: qsTr("Switch Delegate")
        }

        StopButton {
            id: stopButton
            x: 254
            y: 87
            width: 300
            height: 100
            text: "My Button"
            scale: 0.7
            clip: false
            autoRepeat: false
            autoExclusive: false
            checked: false
            checkable: false
            display: AbstractButton.IconOnly
            highlighted: false
        }
    }
}
