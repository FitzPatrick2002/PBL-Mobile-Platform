import open3d as o3d

class MenuDialog:
    def __init__(self, parent_window, callback):
        self.callback = callback
        self.window = parent_window
        self.dialog = o3d.visualization.gui.Dialog("Downsampling Settings")
        self.panel = o3d.visualization.gui.Vert(10,o3d.visualization.gui.Margins(10,10,10,10))

        self.mode_row = o3d.visualization.gui.Horiz(6)
        # Combobox
        self.mode_combo = o3d.visualization.gui.Combobox()
        for i in range(3):
            self.mode_combo.add_item(f"Mode {i}")
        self.mode_row.add_child(o3d.visualization.gui.Label("Mode"))
        self.mode_row.add_child(self.mode_combo)

        self.panel.add_child(self.mode_row)


        self.panel_percent = o3d.visualization.gui.Horiz(6)
        self.panel_lines = o3d.visualization.gui.Horiz(6)

        # Percent slider
        self.percent_slider = o3d.visualization.gui.Slider(o3d.visualization.gui.Slider.INT)
        self.percent_slider.set_limits(0, 100)
        self.percent_slider.int_value = 50
        self.percent_slider.tooltip = "For mode 0 - percent of filtered out points."
        self.panel_percent.add_child(o3d.visualization.gui.Label("Percent"))
        self.panel_percent.add_child(self.percent_slider)

        # Modes 1 and 2, Lines
        self.line_number = o3d.visualization.gui.NumberEdit(o3d.visualization.gui.NumberEdit.INT)
        self.line_number.set_limits(1, 1000)
        self.line_number.int_value = 2
        self.line_number.tooltip = "For modes 1 and 2 - number of lines to filter"
        self.line_number.set_preferred_width(80)
        self.panel_lines.add_child(o3d.visualization.gui.Label("Lines"))
        self.panel_lines.add_child(self.line_number)

        # Button
        self.button_row = o3d.visualization.gui.Horiz()
        btn = o3d.visualization.gui.Button("Run")
        btn.set_on_clicked(self.on_submit)
        self.button_row.add_child(btn)


        self.panel.add_child(self.panel_lines)
        self.panel.add_child(self.panel_percent)
        self.panel.add_child(self.button_row)

        self.dialog.add_child(self.panel)
        #self.dialog.add_child(self.panel_percent)
        #self.dialog.add_child(self.panel_lines)

        self.panel_percent.visible = True
        self.panel_lines.visible = False

        # def on_combo_change(index, text):
        #     self.panel.remove_child(self.panel_percent)
        #     self.panel.remove_child(self.panel_lines)
        #
        #     if index == 0:
        #         self.panel.add_child(self.panel_percent)
        #     else:
        #         self.panel.add_child(self.panel_lines)
        #     # self.panel_percent.visible = (index == 0)
        #     # self.panel_lines.visible = (index in (1,2))
        #     # self.dialog.resize(self.dialog.width, self.dialog.height)
        #
        # self.mode_combo.set_on_selection_changed(on_combo_change)


        # Show dialog attached to the main window
        parent_window.show_dialog(self.dialog)

    def on_submit(self):
        percent = self.percent_slider.int_value
        mode = self.mode_combo.selected_index
        self.callback(percent, mode)