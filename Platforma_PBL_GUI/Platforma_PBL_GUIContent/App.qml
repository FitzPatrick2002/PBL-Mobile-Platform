import QtQuick
import Platforma_PBL_GUI

Window {
    id: appWindow
    visible: true
    title: "Platforma_PBL_GUI"

    // który ekran jest aktualnie wyświetlany
    property string currentScreen: "screen1"

    // szerokość i wysokość dopasowane do aktywnego ekranu
    width:  currentScreen === "screen1" ? screen01.width  : screen02.width
    height: currentScreen === "screen1" ? screen01.height : screen02.height

    // pierwszy ekran
    Screen01 {
        id: screen01
        visible: appWindow.currentScreen === "screen1"
    }

    // drugi ekran
    Screen02 {
        id: screen02
        visible: appWindow.currentScreen === "screen2"
    }

    // OGÓLNA FUNKCJA – przełączanie ekranów
    function openScreen(name) {
        if (name === "screen1" || name === "screen2") {
            currentScreen = name
        } else {
            console.warn("Nieznany ekran:", name)
        }
    }
}
