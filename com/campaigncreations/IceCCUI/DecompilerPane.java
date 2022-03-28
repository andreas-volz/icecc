/*
 * DecompilerPane
 *
 * Author: Jeff Pang <jp@magnus99.dhs.org>
 *
 * CVS: $Id: DecompilerPane.java,v 1.3 2002/06/08 10:12:41 jp Exp $
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
import javax.swing.event.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;

/**
 * Panel for the decompiler functions of icedc.
 */
public class DecompilerPane extends JPanel implements ActionListener {

  // UI Components
  private JList images;
  private JList sprites;
  private JList flingy;
  private JList units;

  private JTextField iscriptIDs;
  private JTextField open;
  private JTextField saveTo;

  private JCheckBox useDefault;
  private JCheckBox separateHeaders;

  private JButton saveToButton;
  private JButton decompileButton;
  private JButton openWithEditorButton;

  // Other components
  private JFrame     owner;
  private Properties prefs;

  /**
   * Create a new decompiler pane with the perferences in prefs
   */
  public DecompilerPane(JFrame owner) {
    this.prefs = IceCCUtil.getPrefs();
    this.owner = owner;

    setLayout(new BorderLayout());
    setBorder(BorderFactory.createEmptyBorder(5,5,5,5));
    add(initLists(), BorderLayout.CENTER);
    JPanel pane2 = new JPanel(new BorderLayout());
    pane2.add(initOptions(), BorderLayout.NORTH);
    pane2.add(initActions(), BorderLayout.SOUTH);
    add(pane2, BorderLayout.SOUTH);
  }

  private JPanel initLists() {
    JPanel mainPane = new JPanel();
    mainPane.setLayout(new BorderLayout());
    mainPane.setBorder(BorderFactory.createEmptyBorder(5,5,5,5)); 

    JPanel topPane = new JPanel();
    topPane.setLayout(new GridLayout(1,4));
    topPane.add(initList("Images", IceCCUtil.IMAGES_LST, images = new JList()));
    topPane.add(initList("Sprites", IceCCUtil.SPRITES_LST, sprites = new JList()));
    topPane.add(initList("Flingy", IceCCUtil.FLINGY_LST, flingy = new JList()));
    topPane.add(initList("Units", IceCCUtil.UNITS_LST, units = new JList()));
    mainPane.add(topPane, BorderLayout.CENTER);

    JPanel bottomPane = new JPanel();
    bottomPane.setLayout(new BorderLayout());
    bottomPane.add(new JLabel("Additional iscript IDs: "), BorderLayout.WEST);
    iscriptIDs = new JTextField();
    bottomPane.add(iscriptIDs, BorderLayout.CENTER);
    mainPane.add(bottomPane, BorderLayout.SOUTH);

    return mainPane;
  }

  private JPanel initList(String title, String listFile, JList assign) {

    class ToggleSelectionModel extends DefaultListSelectionModel {
      
      public void setSelectionInterval(int index0, int index1) {
	// only 1 changed: just toggle
	if (index0 == index1) {
	  if (isSelectedIndex(index0))
	    super.removeSelectionInterval(index0, index0);
	  else
	    super.addSelectionInterval(index0, index0);
	  return;
	}

	// else go through and toggle all to match first selected one
	if (isSelectedIndex(index0)) {
	  super.addSelectionInterval(index0, index1);
	}
	else {
	  super.removeSelectionInterval(index0, index1);
	}
      }
    }

    class ClearAllActionListener implements ActionListener {
      private JList list;

      public ClearAllActionListener(JList list) {
	this.list = list;
      }

      public void actionPerformed(ActionEvent e) {
	list.clearSelection();
      }
    }

    JPanel pane = new JPanel();
    pane.setLayout(new BorderLayout());
    pane.add(new JLabel(title), BorderLayout.NORTH);

    DefaultListModel listModel = new DefaultListModel();

    String fullListFilePath = 
      prefs.getProperty(IceCCUtil.PREF_CONFIG_DIR) + File.separator + listFile;
    
    try {
      BufferedReader reader = 
	new BufferedReader(new FileReader(fullListFilePath));

      String line;
      int index = 0;
      while ((line = reader.readLine()) != null) {
	listModel.addElement(index++ + " " + line);
      }

    } catch (FileNotFoundException e) {
      JOptionPane.showMessageDialog(this, "Could not open file: " + fullListFilePath, 
				    "Error", JOptionPane.ERROR_MESSAGE);
    } catch (IOException e) {
      JOptionPane.showMessageDialog(this, "Error occurred reading file: " + fullListFilePath, 
				    "Error", JOptionPane.ERROR_MESSAGE);
    }

    assign.setModel(listModel);
    assign.setSelectionModel(new ToggleSelectionModel());
    pane.add(new JScrollPane(assign), BorderLayout.CENTER);
    pane.setPreferredSize(new Dimension(150, 300));

    JButton clear = new JButton("Unselect All");
    clear.addActionListener(new ClearAllActionListener(assign));
    pane.add(clear, BorderLayout.SOUTH);

    return pane;
  }

  private JPanel initOptions() {
    JPanel mainPane = new JPanel();
    mainPane.setLayout(new BorderLayout());
    mainPane.setBorder(BorderFactory.createEmptyBorder(5,5,5,5));

    JPanel middlePane = new JPanel();
    middlePane.setLayout(new BorderLayout());

    useDefault = new JCheckBox("Use default iscript.bin");
    separateHeaders = new JCheckBox("Separate headers");
    middlePane.add(useDefault, BorderLayout.WEST);
    middlePane.add(separateHeaders, BorderLayout.CENTER);

    open = new JTextField();
    saveTo = new JTextField();

    JPanel bottom1Pane = IceCCUtil.makeFileSelectionPane("Open: ", open,
							 FileDialog.LOAD);
    JPanel bottom2Pane = IceCCUtil.makeFileSelectionPane("Save to: ", saveTo,
							 FileDialog.SAVE);

    class UseDefaultChangeListener implements ChangeListener {
      JPanel filePane;
      JCheckBox sel;

      public UseDefaultChangeListener(JCheckBox b, JPanel p) {
	filePane = p;
	sel = b;
      }

      public void stateChanged(ChangeEvent e) {
	filePane.setEnabled(!sel.isSelected());
      }
    }

    useDefault.addChangeListener(new UseDefaultChangeListener(useDefault, bottom1Pane));
    useDefault.setSelected(prefs.getProperty(IceCCUtil.PREF_USE_DEFAULT_ISCRIPT).equals("true"));
    separateHeaders.setSelected(prefs.getProperty(IceCCUtil.PREF_SEPARATE_HEADERS).equals("true"));

    mainPane.add(middlePane, BorderLayout.NORTH);
    JPanel bottomPane = new JPanel(new GridLayout(2,1));
    bottomPane.add(bottom1Pane);
    bottomPane.add(bottom2Pane);
    mainPane.add(bottomPane, BorderLayout.SOUTH);

    return mainPane;
  }

  private JPanel initActions() {
    JPanel pane = new JPanel();
    pane.setLayout(new FlowLayout(FlowLayout.RIGHT));
    
    decompileButton = new JButton("Decompile");
    decompileButton.addActionListener(this);
    openWithEditorButton = new JButton("Open in Editor");
    openWithEditorButton.addActionListener(this);
    
    pane.add(decompileButton);
    pane.add(openWithEditorButton);

    return pane;
  }

  public void actionPerformed(ActionEvent e) {
    Object source = e.getSource();
    if (source == decompileButton) {
      handleDecompile();
    } else if (source == openWithEditorButton) {
      handleOpenWithEditor();
    }
  }

  private void handleDecompile() {
    Vector cmd = new Vector();
    cmd.add(prefs.getProperty(IceCCUtil.PREF_ICEDC_EXEC));

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

    int[] imagesSelected = images.getSelectedIndices();
    int[] spritesSelected = sprites.getSelectedIndices();
    int[] flingySelected = flingy.getSelectedIndices();
    int[] unitsSelected = units.getSelectedIndices();

    if (imagesSelected.length > 0) {
      cmd.add("-m");
      StringBuffer list = new StringBuffer(""+imagesSelected[0]);
      for (int i=1; i<imagesSelected.length; i++)
	list.append("," + imagesSelected[i]);
      cmd.add(list.toString());
    }
    if (spritesSelected.length > 0) {
      cmd.add("-p");
      StringBuffer list = new StringBuffer(""+spritesSelected[0]);
      for (int i=1; i<spritesSelected.length; i++)
	list.append("," + spritesSelected[i]);
      cmd.add(list.toString());
    }
    if (flingySelected.length > 0) {
      cmd.add("-f");
      StringBuffer list = new StringBuffer(""+flingySelected[0]);
      for (int i=1; i<flingySelected.length; i++)
	list.append("," + flingySelected[i]);
      cmd.add(list.toString());
    }
    if (unitsSelected.length > 0) {
      cmd.add("-u");
      StringBuffer list = new StringBuffer(""+unitsSelected[0]);
      for (int i=1; i<unitsSelected.length; i++)
	list.append("," + unitsSelected[i]);
      cmd.add(list.toString());
    }

    String ids = iscriptIDs.getText();
    // ignore leading space
    while (ids.length() > 0 && (ids.charAt(0) == ' ' || ids.charAt(0) == ',' || ids.charAt(0) == ';'))
      ids = ids.substring(1,ids.length()-1);
    // put in quotes in case user used spaces for the list instead of commas
    if (ids.length() > 0) {
      cmd.add("-i");
      cmd.add(ids);
    }

    if (separateHeaders.isSelected())
      cmd.add("-s");

    if (useDefault.isSelected())
      cmd.add("-d");
    else if (open.getText().length() > 0)
      cmd.add(open.getText());
    
    Object[] oargs = cmd.toArray();
    String[] args = new String[oargs.length];
    for (int i=0; i<oargs.length; i++)
      args[i] = (String)oargs[i];
    IceCCUtil.execute(owner, args, true);
  }

  private void handleOpenWithEditor() {
    IceCCUtil.execute(owner, new String[] { prefs.getProperty(IceCCUtil.PREF_TEXT_EDITOR),
		      (saveTo.getText().length() > 0 ? saveTo.getText() :
		      IceCCUtil.DEFAULT_ICEDC_OUTPUT_FILE) }, false);
  }
  
}
