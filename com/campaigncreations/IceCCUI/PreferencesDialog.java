/*
 * PreferencesDialog
 *
 * Author: Jeff Pang <jp@magnus99.dhs.org>
 *
 * CVS: $Id: PreferencesDialog.java,v 1.2 2002/06/08 10:12:42 jp Exp $
 *
 * IceCCUI. Graphical user interface component for IceCC
 * Copyright (C) 2001 Jeffrey Pang <jp@magnus99.dhs.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

package com.campaigncreations.IceCCUI;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;

/**
 * This is a small dialog to handle changes to preferences.
 */
public class PreferencesDialog extends JDialog implements ActionListener {

  private JTextField icecc, icedc, confFile, confDir, editor;
  private JButton cancel, save;

  private Properties prefs;

  public PreferencesDialog(JFrame owner) {
    super(owner, "Preferences");
    prefs = IceCCUtil.getPrefs();
    getContentPane().add(initMainPanel());
    pack();
  }

  private JPanel initMainPanel() {
    JPanel pane = new JPanel(new BorderLayout());
    pane.setBorder(BorderFactory.createEmptyBorder(8,8,8,8));

    JPanel topPane = new JPanel(new GridLayout(5, 1));
    icecc = new JTextField();
    icedc = new JTextField();
    confFile = new JTextField();
    confDir = new JTextField();
    editor = new JTextField();

    FileDialog sharedDialog = new FileDialog(IceCCUI.rootFrame,
					     "Select",
					     FileDialog.LOAD);

    topPane.add(IceCCUtil.makeFileSelectionPane("icecc Executable: ", icecc, sharedDialog));
    topPane.add(IceCCUtil.makeFileSelectionPane("icedc Executable: ", icedc, sharedDialog));
    topPane.add(IceCCUtil.makeFileSelectionPane("IceCC Config File: ", confFile, sharedDialog));
    topPane.add(IceCCUtil.makeFileSelectionPane("IceCC Config Dir: ", confDir, sharedDialog));
    topPane.add(IceCCUtil.makeFileSelectionPane("Text Editor: ", editor, sharedDialog));
    icecc.setText(prefs.getProperty(IceCCUtil.PREF_ICECC_EXEC));
    icedc.setText(prefs.getProperty(IceCCUtil.PREF_ICEDC_EXEC));
    confFile.setText(prefs.getProperty(IceCCUtil.PREF_CONFIG_FILE));
    confDir.setText(prefs.getProperty(IceCCUtil.PREF_CONFIG_DIR));
    editor.setText(prefs.getProperty(IceCCUtil.PREF_TEXT_EDITOR));
    
    JPanel bottomPane = new JPanel(new FlowLayout(FlowLayout.RIGHT));
    save = new JButton("Save");
    save.addActionListener(this);
    cancel = new JButton("Cancel");
    cancel.addActionListener(this);
    bottomPane.add(save);
    bottomPane.add(cancel);

    pane.add(topPane, BorderLayout.NORTH);
    pane.add(bottomPane, BorderLayout.SOUTH);
    pane.setPreferredSize(new Dimension(500,200));

    return pane;
  }

  public void actionPerformed(ActionEvent e) {
    Object source = e.getSource();
    if (source == cancel) {
      dispose();
      return;
    } else if (source == save) {
      prefs.setProperty(IceCCUtil.PREF_ICECC_EXEC, icecc.getText());
      prefs.setProperty(IceCCUtil.PREF_ICEDC_EXEC, icedc.getText());
      prefs.setProperty(IceCCUtil.PREF_CONFIG_FILE, confFile.getText());
      prefs.setProperty(IceCCUtil.PREF_CONFIG_DIR, confDir.getText());
      prefs.setProperty(IceCCUtil.PREF_TEXT_EDITOR, editor.getText());
      IceCCUtil.savePrefs();
      dispose();
      return;
    }
  }
}
