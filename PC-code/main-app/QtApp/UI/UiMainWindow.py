# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'test-window.ui'
##
## Created by: Qt User Interface Compiler version 6.10.1
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide6.QtCore import (QCoreApplication, QDate, QDateTime, QLocale,
    QMetaObject, QObject, QPoint, QRect,
    QSize, QTime, QUrl, Qt)
from PySide6.QtGui import (QBrush, QColor, QConicalGradient, QCursor,
    QFont, QFontDatabase, QGradient, QIcon,
    QImage, QKeySequence, QLinearGradient, QPainter,
    QPalette, QPixmap, QRadialGradient, QTransform)
from PySide6.QtWidgets import (QApplication, QComboBox, QDial, QFrame,
    QGridLayout, QHBoxLayout, QLCDNumber, QLabel,
    QLineEdit, QMainWindow, QMenuBar, QPushButton,
    QSizePolicy, QStatusBar, QVBoxLayout, QWidget)

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        if not MainWindow.objectName():
            MainWindow.setObjectName(u"MainWindow")
        MainWindow.resize(800, 600)
        self.centralwidget = QWidget(MainWindow)
        self.centralwidget.setObjectName(u"centralwidget")
        self.layoutWidget = QWidget(self.centralwidget)
        self.layoutWidget.setObjectName(u"layoutWidget")
        self.layoutWidget.setGeometry(QRect(130, 60, 516, 167))
        self.horizontalLayout = QHBoxLayout(self.layoutWidget)
        self.horizontalLayout.setObjectName(u"horizontalLayout")
        self.horizontalLayout.setContentsMargins(0, 0, 0, 0)
        self.gridLayout_5 = QGridLayout()
        self.gridLayout_5.setObjectName(u"gridLayout_5")
        self.gridLayout_2 = QGridLayout()
        self.gridLayout_2.setObjectName(u"gridLayout_2")
        self.x_pos_label = QLabel(self.layoutWidget)
        self.x_pos_label.setObjectName(u"x_pos_label")

        self.gridLayout_2.addWidget(self.x_pos_label, 0, 0, 1, 1)

        self.x_pos_display = QLCDNumber(self.layoutWidget)
        self.x_pos_display.setObjectName(u"x_pos_display")

        self.gridLayout_2.addWidget(self.x_pos_display, 0, 1, 1, 1)

        self.y_pos_label = QLabel(self.layoutWidget)
        self.y_pos_label.setObjectName(u"y_pos_label")

        self.gridLayout_2.addWidget(self.y_pos_label, 1, 0, 1, 1)

        self.y_pos_display = QLCDNumber(self.layoutWidget)
        self.y_pos_display.setObjectName(u"y_pos_display")

        self.gridLayout_2.addWidget(self.y_pos_display, 1, 1, 1, 1)

        self.label_3 = QLabel(self.layoutWidget)
        self.label_3.setObjectName(u"label_3")

        self.gridLayout_2.addWidget(self.label_3, 2, 0, 1, 1)

        self.angle_display = QLCDNumber(self.layoutWidget)
        self.angle_display.setObjectName(u"angle_display")

        self.gridLayout_2.addWidget(self.angle_display, 2, 1, 1, 1)


        self.gridLayout_5.addLayout(self.gridLayout_2, 0, 0, 1, 1)

        self.front_collision_frame = QFrame(self.layoutWidget)
        self.front_collision_frame.setObjectName(u"front_collision_frame")
        self.front_collision_frame.setAutoFillBackground(False)
        self.front_collision_frame.setStyleSheet(u"background-color: rgb(0, 200, 0);")
        self.front_collision_frame.setFrameShape(QFrame.StyledPanel)
        self.front_collision_frame.setFrameShadow(QFrame.Raised)

        self.gridLayout_5.addWidget(self.front_collision_frame, 1, 0, 1, 1)

        self.back_collision_frame = QFrame(self.layoutWidget)
        self.back_collision_frame.setObjectName(u"back_collision_frame")
        self.back_collision_frame.setAutoFillBackground(False)
        self.back_collision_frame.setStyleSheet(u"background-color: rgb(0, 200, 0);")
        self.back_collision_frame.setFrameShape(QFrame.StyledPanel)
        self.back_collision_frame.setFrameShadow(QFrame.Raised)

        self.gridLayout_5.addWidget(self.back_collision_frame, 2, 0, 1, 1)


        self.horizontalLayout.addLayout(self.gridLayout_5)

        self.verticalLayout = QVBoxLayout()
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.dial = QDial(self.layoutWidget)
        self.dial.setObjectName(u"dial")

        self.verticalLayout.addWidget(self.dial)

        self.gridLayout = QGridLayout()
        self.gridLayout.setObjectName(u"gridLayout")

        self.calibration_btn = QPushButton(self.layoutWidget)
        self.calibration_btn.setObjectName(u"calibration_btn")

        self.gridLayout.addWidget(self.calibration_btn, 1, 0, 1, 1)

        self.forward_ctrl_btn = QPushButton(self.layoutWidget)
        self.forward_ctrl_btn.setObjectName(u"forward_ctrl_btn")

        self.gridLayout.addWidget(self.forward_ctrl_btn, 0, 1, 1, 1)

        self.backward_ctrl_btn = QPushButton(self.layoutWidget)
        self.backward_ctrl_btn.setObjectName(u"backward_ctrl_btn")

        self.gridLayout.addWidget(self.backward_ctrl_btn, 1, 1, 1, 1)

        self.left_ctrl_button = QPushButton(self.layoutWidget)
        self.left_ctrl_button.setObjectName(u"left_ctrl_button")

        self.gridLayout.addWidget(self.left_ctrl_button, 0, 0, 1, 1)

        self.right_ctrl_btn = QPushButton(self.layoutWidget)
        self.right_ctrl_btn.setObjectName(u"right_ctrl_btn")

        self.gridLayout.addWidget(self.right_ctrl_btn, 0, 2, 1, 1)


        self.verticalLayout.addLayout(self.gridLayout)


        self.horizontalLayout.addLayout(self.verticalLayout)

        self.gridLayout_4 = QGridLayout()
        self.gridLayout_4.setObjectName(u"gridLayout_4")
        self.new_session_btn = QPushButton(self.layoutWidget)
        self.new_session_btn.setObjectName(u"new_session_btn")

        self.gridLayout_4.addWidget(self.new_session_btn, 0, 0, 1, 1)

        self.scan_btn = QPushButton(self.layoutWidget)
        self.scan_btn.setObjectName(u"scan_btn")

        self.gridLayout_4.addWidget(self.scan_btn, 4, 0, 1, 1)

        self.session_select = QComboBox(self.layoutWidget)
        self.session_select.setObjectName(u"session_select")

        self.gridLayout_4.addWidget(self.session_select, 2, 0, 1, 1)

        self.gridLayout_3 = QGridLayout()
        self.gridLayout_3.setObjectName(u"gridLayout_3")
        self.scans_number_label = QLabel(self.layoutWidget)
        self.scans_number_label.setObjectName(u"scans_number_label")
        self.scans_number_label.setMaximumSize(QSize(16777215, 10))

        self.gridLayout_3.addWidget(self.scans_number_label, 0, 0, 1, 1)

        self.every_nth_label = QLabel(self.layoutWidget)
        self.every_nth_label.setObjectName(u"every_nth_label")
        self.every_nth_label.setMaximumSize(QSize(16777215, 10))

        self.gridLayout_3.addWidget(self.every_nth_label, 0, 1, 1, 1)

        self.scans_number_selector = QComboBox(self.layoutWidget)
        self.scans_number_selector.addItem("")
        self.scans_number_selector.addItem("")
        self.scans_number_selector.addItem("")
        self.scans_number_selector.addItem("")
        self.scans_number_selector.addItem("")
        self.scans_number_selector.addItem("")
        self.scans_number_selector.addItem("")
        self.scans_number_selector.addItem("")
        self.scans_number_selector.setObjectName(u"scans_number_selector")

        self.gridLayout_3.addWidget(self.scans_number_selector, 1, 0, 1, 1)

        self.every_nth_selector = QComboBox(self.layoutWidget)
        self.every_nth_selector.addItem("")
        self.every_nth_selector.addItem("")
        self.every_nth_selector.addItem("")
        self.every_nth_selector.addItem("")
        self.every_nth_selector.addItem("")
        self.every_nth_selector.addItem("")
        self.every_nth_selector.setObjectName(u"every_nth_selector")

        self.gridLayout_3.addWidget(self.every_nth_selector, 1, 1, 1, 1)


        self.gridLayout_4.addLayout(self.gridLayout_3, 3, 0, 1, 1)

        self.new_session_name_edit = QLineEdit(self.layoutWidget)
        self.new_session_name_edit.setObjectName(u"new_session_name_edit")

        self.gridLayout_4.addWidget(self.new_session_name_edit, 1, 0, 1, 1)


        self.horizontalLayout.addLayout(self.gridLayout_4)

        MainWindow.setCentralWidget(self.centralwidget)
        self.menubar = QMenuBar(MainWindow)
        self.menubar.setObjectName(u"menubar")
        self.menubar.setGeometry(QRect(0, 0, 800, 17))
        MainWindow.setMenuBar(self.menubar)
        self.statusbar = QStatusBar(MainWindow)
        self.statusbar.setObjectName(u"statusbar")
        MainWindow.setStatusBar(self.statusbar)

        self.retranslateUi(MainWindow)

        QMetaObject.connectSlotsByName(MainWindow)
    # setupUi

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(QCoreApplication.translate("MainWindow", u"MainWindow", None))
        self.x_pos_label.setText(QCoreApplication.translate("MainWindow", u"X:", None))
        self.y_pos_label.setText(QCoreApplication.translate("MainWindow", u"Y:", None))
        self.label_3.setText(QCoreApplication.translate("MainWindow", u"A:", None))
        self.calibration_btn.setText(QCoreApplication.translate("MainWindow", u"Calibrate", None))
        self.forward_ctrl_btn.setText(QCoreApplication.translate("MainWindow", u"Forward", None))
        self.backward_ctrl_btn.setText(QCoreApplication.translate("MainWindow", u"Back", None))
        self.left_ctrl_button.setText(QCoreApplication.translate("MainWindow", u"Left", None))
        self.right_ctrl_btn.setText(QCoreApplication.translate("MainWindow", u"Right", None))
        self.new_session_btn.setText(QCoreApplication.translate("MainWindow", u"New Session", None))
        self.scan_btn.setText(QCoreApplication.translate("MainWindow", u"Scan", None))
        self.scans_number_label.setText(QCoreApplication.translate("MainWindow", u"Scans Number", None))
        self.every_nth_label.setText(QCoreApplication.translate("MainWindow", u"Every n-th", None))
        self.scans_number_selector.setItemText(0, QCoreApplication.translate("MainWindow", u"1", None))
        self.scans_number_selector.setItemText(1, QCoreApplication.translate("MainWindow", u"2", None))
        self.scans_number_selector.setItemText(2, QCoreApplication.translate("MainWindow", u"3", None))
        self.scans_number_selector.setItemText(3, QCoreApplication.translate("MainWindow", u"4", None))
        self.scans_number_selector.setItemText(4, QCoreApplication.translate("MainWindow", u"5", None))
        self.scans_number_selector.setItemText(5, QCoreApplication.translate("MainWindow", u"10", None))
        self.scans_number_selector.setItemText(6, QCoreApplication.translate("MainWindow", u"15", None))
        self.scans_number_selector.setItemText(7, QCoreApplication.translate("MainWindow", u"20", None))

        self.every_nth_selector.setItemText(0, QCoreApplication.translate("MainWindow", u"1", None))
        self.every_nth_selector.setItemText(1, QCoreApplication.translate("MainWindow", u"5", None))
        self.every_nth_selector.setItemText(2, QCoreApplication.translate("MainWindow", u"10", None))
        self.every_nth_selector.setItemText(3, QCoreApplication.translate("MainWindow", u"15", None))
        self.every_nth_selector.setItemText(4, QCoreApplication.translate("MainWindow", u"25", None))
        self.every_nth_selector.setItemText(5, QCoreApplication.translate("MainWindow", u"50", None))

        self.new_session_name_edit.setText("")
    # retranslateUi

