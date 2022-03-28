/*
 * CompilerPane
 *
 * Author: Jeff Pang <jp@magnus99.dhs.org>
 *
 * CVS: $Id: CompilerPane.java,v 1.4 2002/06/09 03:16:45 jp Exp $
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
 * This is the panel with the compiler functions
 */
public class CompilerPane extends JPanel implements ActionListener {
  
  // UI Components
  private JList filesList;
  private DefaultListModel files;
  private JButton add, remove, up, down, open;
  private JCheckBox merge, wall;
  private JTextField saveTo;
  private JButton compile;
  
  private FileDialog addFileDialog;

  // Other Components
  private JFrame owner;
  private Properties prefs;

  public CompilerPane(JFrame owner) {
    this.owner = owner;
    prefs = IceCCUtil.getPrefs();

    setLayout(new BorderLayout());
    setBorder(BorderFactory.createEmptyBorder(5,5,5,5));
    
    add(initFilesList(), BorderLayout.CENTER);
    JPanel pane2 = new JPanel(new BorderLayout());
    pane2.add(initOptions(), BorderLayout.NORTH);
    pane2.add(initActions(), BorderLayout.SOUTH);
    add(pane2, BorderLayout.SOUTH);

    addFileDialog = new FileDialog(IceCCUI.rootFrame, "Select", 
				   FileDialog.LOAD);
  }

  private JPanel initFilesList() {

    JPanel pane = new JPanel(new BorderLayout());
    pane.setBorder(BorderFactory.createEmptyBorder(5,5,5,5));

    files = new DefaultListModel();
    filesList = new JList(files);
    JScrollPane middlePane = new JScrollPane(filesList);
    middlePane.setPreferredSize(new Dimension(600, 200));
    
    JPanel bottomPane = new JPanel(new GridLayout(1,5));
    add = new JButton("Add");
    add.addActionListener(this);
    remove = new JButton("Remove");
    remove.addActionListener(this);
    up = new JButton("Move Up");
    up.addActionListener(this);
    down = new JButton("Move Down");
    down.addActionListener(this);
    open = new JButton("Open");
    open.addActionListener(this);
    bottomPane.add(add);
    bottomPane.add(remove);
    bottomPane.add(up);
    bottomPane.add(down);
    bottomPane.add(open);
    
    pane.add(new JLabel("Source Files"), BorderLayout.NORTH);
    pane.add(middlePane, BorderLayout.CENTER);
    pane.add(bottomPane, BorderLayout.SOUTH);

    return pane;
  }

  private JPanel initOptions() {
    JPanel pane = new JPanel(new BorderLayout());
    pane.setBorder(BorderFactory.createEmptyBorder(5,5,5,5));

    JPanel topPane = new JPanel(new FlowLayout());
    merge = new JCheckBox("Merge with default iscript.bin");
    wall = new JCheckBox("Display all warnings");
    merge.setSelected(prefs.getProperty(IceCCUtil.PREF_MERGE_DEFAULT_ISCRIPT).equals("true"));
    wall.setSelected(prefs.getProperty(IceCCUtil.PREF_DISPLAY_ALL_WARNINGS).equals("true"));
    topPane.add(merge);
    topPane.add(wall);

    saveTo = new JTextField();
    JPanel bottomPane = IceCCUtil.makeFileSelectionPane("Save to: ", saveTo,
							FileDialog.SAVE);
    
    pane.add(topPane, BorderLayout.CENTER);
    pane.add(bottomPane, BorderLayout.SOUTH);
    return pane;
  }
    
  private JPanel initActions() {
    JPanel pane = new JPanel(new FlowLayout(FlowLayout.RIGHT));
    compile = new JButton("Compile");
    compile.addActionListener(this);
    pane.add(compile);
    return pane;
  }

  public void actionPerformed(ActionEvent e) {
    Object source = e.getSource();

    if (source == compile) {
      handleCompile();
    } else if (source == add) {      
      addFileDialog.show();
      if (addFileDialog.getFile() != null &&
	  addFileDialog.getFile().length() > 0) {
	String dir = addFileDialog.getDirectory();
	if (dir.length() > 0) {
	  addFileDialog.setDirectory(dir);
	}
	files.addElement(dir + addFileDialog.getFile());
      }
    } else if (source == remove) {
      int[] sel = filesList.getSelectedIndices();
      if (sel.length > 0) {
	for (int i=0; i<sel.length; i++) {
	  files.remove(sel[i]-i);
	}
      }
    } else if (source == up) {
      int sel = filesList.getSelectedIndex();
      if (sel > 0) {
	files.insertElementAt(files.getElementAt(sel), sel-1);
	files.removeElementAt(sel+1);
	filesList.setSelectedIndex(sel-1);
      }
    } else if (source == down) {
      int sel = filesList.getSelectedIndex();
      if (sel != -1 && sel < files.getSize()-1) {
	files.insertElementAt(files.getElementAt(sel), sel+2);
	files.removeElementAt(sel);
	filesList.setSelectedIndex(sel+1);
      }
    } else if (source == open) {
      String file = (String)filesList.getSelectedValue();
      if (file != null) {
	IceCCUtil.execute(owner, new String[] { prefs.getProperty(IceCCUtil.PREF_TEXT_EDITOR),
						file }, false);
      }
    }
  }

  private void handleCompile() {
    Object[] values = files.toArray();
    if (values.length == 0) {
      JOptionPane.showMessageDialog(this, "You must choose some source files to compile.", "Error", 
				    JOptionPane.WARNING_MESSAGE);
      return;
    }

    Vector cmd = new Vector();
    cmd.add(prefs.getProperty(IceCCUtil.PREF_ICECC_EXEC));

    String confFile = prefs.getProperty(IceCCUtil.PREF_CONFIG_FILE);
    String confDir = prefs.getProperty(IceCCUtil.PREF_CONFIG_DIR);
    if (confFile.length() > 0) {
      cmd.add("-c");
      cmd.add(confFile);
    }
    if (confDir.length() > 0) {
      cmd.add("-r");
      cmd.add(confDir);
    }
    
    if (saveTo.getText().length() > 0) {
      cmd.add("-o");
      cmd.add(saveTo.getText());
    }

    if (merge.isSelected())
      cmd.add("-m");
    if (wall.isSelected())
      cmd.add("-w");

    for (int i=0; i<values.length; i++)
      cmd.add(values[i]);

    values = cmd.toArray();
    String[] args = new String[values.length];
    for (int i=0; i<values.length; i++)
      args[i] = (String)values[i];

    IceCCUtil.execute(owner, args, true);
  }

}
