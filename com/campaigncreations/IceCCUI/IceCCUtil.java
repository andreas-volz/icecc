/*
 * IceCCUtil
 *
 * Author: Jeff Pang <jp@magnus99.dhs.org>
 *
 * CVS: $Id: IceCCUtil.java,v 1.5 2002/06/09 05:16:29 jp Exp $
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
 * Utility functions and constants for IceCCUI
 */
public class IceCCUtil {

  // data file names
  public static final String IMAGES_LST = "images.lst";
  public static final String SPRITES_LST = "sprites.lst";
  public static final String FLINGY_LST = "flingy.lst";
  public static final String UNITS_LST = "units.lst";

  // other constants
  public static final String DEFAULT_ICEDC_OUTPUT_FILE = "iscript.txt";
  public static final String DEFAULT_ICECC_OUTPUT_FILE = "iscript.bin";
  public static final String ICECCUI_CONFIG_FILE = "iceccui.ini";

  // preferece variables names
  public static final String PREF_USE_DEFAULT_ISCRIPT = "UseDefaultIscript";
  public static final String PREF_SEPARATE_HEADERS = "SeparateHeaders";
  public static final String PREF_MERGE_DEFAULT_ISCRIPT = "MergeDefaultIscript";
  public static final String PREF_DISPLAY_ALL_WARNINGS = "DisplayAllWarnings";
  public static final String PREF_ICECC_EXEC = "IceccExec";
  public static final String PREF_ICEDC_EXEC = "IcedcExec";
  public static final String PREF_CONFIG_FILE = "ConfigFile";
  public static final String PREF_CONFIG_DIR = "ConfigDir";
  public static final String PREF_TEXT_EDITOR = "TextEditor";

  // preferece variable default values
  public static final String PREF_USE_DEFAULT_ISCRIPT_DEFAULT = "true";
  public static final String PREF_SEPARATE_HEADERS_DEFAULT = "true";
  public static final String PREF_MERGE_DEFAULT_ISCRIPT_DEFAULT = "true";
  public static final String PREF_DISPLAY_ALL_WARNINGS_DEFAULT = "true";
  public static final String PREF_ICECC_EXEC_DEFAULT = "./icecc";
  public static final String PREF_ICEDC_EXEC_DEFAULT = "./icedc";
  public static final String PREF_CONFIG_FILE_DEFAULT = "./icecc.ini";
  public static final String PREF_CONFIG_DIR_DEFAULT = "./data";
  public static final String PREF_TEXT_EDITOR_DEFAULT = "emacs";

  // global preferences object
  public static Properties prefs;
  
  /** 
   * Simple class for displaying an error dialog with a list of messages in
   * the constructor vector (filled with strings)
   */
  private static class ErrorDialog extends JDialog implements ActionListener {

    JButton ok, editor;
    JList msgs;
    JFrame owner;

    public void actionPerformed(ActionEvent e) {
      Object source = e.getSource();
      if (source == ok) {
	dispose();
	return;
      } else if (source == editor) {
	String line = (String)msgs.getSelectedValue();
	if (line == null) return;
	line = IceCCUtil.parseErrorMsgForFile(line);
	if (line == null) return;
	IceCCUtil.execute(owner, new String[] { IceCCUtil.getPrefs().getProperty(IceCCUtil.PREF_TEXT_EDITOR),
			  line }, false);
      }
    }

    public ErrorDialog(JFrame owner, Vector messages) {
      super(owner, "An Error Occurred");
      this.owner = owner;
      JPanel pane = new JPanel(new BorderLayout());
      pane.setBorder(BorderFactory.createEmptyBorder(8,8,8,8));

      msgs = new JList(messages);
      DefaultListSelectionModel model = new DefaultListSelectionModel();
      model.setSelectionMode(DefaultListSelectionModel.SINGLE_SELECTION);
      msgs.setSelectionModel(model);
      JScrollPane msgPane = new JScrollPane(msgs);
      msgPane.setPreferredSize(new Dimension(500, 250));

      JPanel buttonPane = new JPanel(new FlowLayout(FlowLayout.RIGHT));
      ok = new JButton("      OK      ");
      ok.addActionListener(this);
      editor = new JButton("Open in Editor");
      editor.addActionListener(this);
      buttonPane.add(editor);
      buttonPane.add(ok);

      pane.add(msgPane, BorderLayout.CENTER);
      pane.add(buttonPane, BorderLayout.SOUTH);
      getContentPane().add(pane);
      pack();
    }
      
  }

  /**
   * Generate a simple error dialog.
   *
   * @see ErrorDialog
   */
  public static void showErrorDialog(JFrame owner, Vector messages) {
    (new ErrorDialog(owner, messages)).setVisible(true);
  }

  /**
   * Get IceCC preferences.
   */
  public static Properties getPrefs() {
    return prefs;
  }

  /**
   * Set IceCC preferences
   */
  public static void setPrefs(Properties p) {
    prefs = p;
  }

  public static void savePrefs() {
    try {
      prefs.store(new FileOutputStream(ICECCUI_CONFIG_FILE), "IceCCUI Configuration File");
    } catch (IOException e) {
      JOptionPane.showMessageDialog(null, "Could not save IceCCUI's configuration file: " +
				    ICECCUI_CONFIG_FILE + ".\n" +
				    "Make sure IceCCUI is allowed to write to it.", "Error",
				    JOptionPane.WARNING_MESSAGE);
    }
  }

  public static void loadPrefs() {
    prefs = new Properties();
    try {
      prefs.load(new FileInputStream(ICECCUI_CONFIG_FILE));
    } catch (FileNotFoundException e) {
      JOptionPane.showMessageDialog(null, "Could not find IceCCUI's configuration file: " +
				    ICECCUI_CONFIG_FILE + ".\n" +
				    "Will try loading the default preferences.", "Error",
				    JOptionPane.WARNING_MESSAGE);
    } catch (IOException e) {
      JOptionPane.showMessageDialog(null, "Error loading IceCCUI's configuration file: " +
				    ICECCUI_CONFIG_FILE + ".\n" +
				    "Make sure the syntax is valid and that it is readable.\n" +
				    "Will try loading the default preferences.", "Error",
				    JOptionPane.WARNING_MESSAGE);
    }

    validatePrefs();
  }

  private static void validatePrefs() {
    if (prefs.getProperty(PREF_USE_DEFAULT_ISCRIPT) == null)
      prefs.setProperty(PREF_USE_DEFAULT_ISCRIPT, PREF_USE_DEFAULT_ISCRIPT_DEFAULT);
    if (prefs.getProperty(PREF_SEPARATE_HEADERS) == null)
      prefs.setProperty(PREF_SEPARATE_HEADERS, PREF_SEPARATE_HEADERS_DEFAULT);
    if (prefs.getProperty(PREF_MERGE_DEFAULT_ISCRIPT) == null)
      prefs.setProperty(PREF_MERGE_DEFAULT_ISCRIPT, PREF_MERGE_DEFAULT_ISCRIPT_DEFAULT);
    if (prefs.getProperty(PREF_DISPLAY_ALL_WARNINGS) == null)
      prefs.setProperty(PREF_DISPLAY_ALL_WARNINGS, PREF_DISPLAY_ALL_WARNINGS_DEFAULT);
    if (prefs.getProperty(PREF_ICECC_EXEC) == null)
      prefs.setProperty(PREF_ICECC_EXEC, PREF_ICECC_EXEC_DEFAULT);
    if (prefs.getProperty(PREF_ICEDC_EXEC) == null)
      prefs.setProperty(PREF_ICEDC_EXEC, PREF_ICEDC_EXEC_DEFAULT);
    if (prefs.getProperty(PREF_CONFIG_FILE) == null)
      prefs.setProperty(PREF_CONFIG_FILE, PREF_CONFIG_FILE_DEFAULT);
    if (prefs.getProperty(PREF_CONFIG_DIR) == null)
      prefs.setProperty(PREF_CONFIG_DIR, PREF_CONFIG_DIR_DEFAULT);
    if (prefs.getProperty(PREF_TEXT_EDITOR) == null)
      prefs.setProperty(PREF_TEXT_EDITOR, PREF_TEXT_EDITOR_DEFAULT);
  }

  /**
   * Make a simple "open file" dialog which fills a text field with the file
   * chosen.
   *
   * @param label a text label to place before the text field
   * @param field the text label to fill with the file name and path
   * @param dialogType one of JFileChooser.OPEN_DIALOG, 
   * JFileChooser.SAVE_DIALOG, JFileChooser.CUSTOM_DIALOG.
   * @return the new panel constructed
   */
  public static JPanel makeFileSelectionPane(String label, 
					     JTextField field,
					     int dialogType) {

    FileDialog fileDialog = 
      new FileDialog(IceCCUI.rootFrame, 
		     dialogType==FileDialog.LOAD?"OPEN":"SAVE",
		     dialogType);
    return makeFileSelectionPane(label, field, fileDialog);
  }

  /**
   * Make a simepl "open file" dialog which may share a JFileChooser with
   * other fields (e.g., so navigation remains consistent).
   */
  public static JPanel makeFileSelectionPane(String label, 
					     JTextField field,
					     JFileChooser fileDialog) 
  {
    class FileSelectionListener implements ActionListener {
      private JTextField field;
      private JFileChooser fileDialog;

      public FileSelectionListener(JTextField field, JFileChooser fdialog) {
	this.field = field;
	this.fileDialog = fdialog;
      }

      public void actionPerformed(ActionEvent e) {
	int retval = fileDialog.showOpenDialog(field);
	if (retval == JFileChooser.APPROVE_OPTION) {
	  String dir = fileDialog.getSelectedFile().getParent();
	  if (dir.length() > 0) {
	    fileDialog.setCurrentDirectory(new File(dir));
	  }
	  field.setText(fileDialog.getSelectedFile().getPath());
	}
      }
    }

    class FileSelectionPane extends JPanel {
      private JButton browse;
      private JTextField field;
      
      public FileSelectionPane(String label, JTextField field, 
			       JFileChooser fdialog) 
      {
	super();
	setLayout(new BorderLayout());
	
	add(new JLabel(label), BorderLayout.WEST);
	add(field, BorderLayout.CENTER);
	this.field = field;

	browse = new JButton("Browse...");
	browse.addActionListener(new FileSelectionListener(field, fdialog));
	add(browse, BorderLayout.EAST);
      }

      public void setEnabled(boolean b) {
	browse.setEnabled(b);
	field.setEnabled(b);
      }
    }

    return new FileSelectionPane(label, field, fileDialog);
  }

  /**
   * Make a simepl "open file" dialog with a native awt FileDialog
   */
  public static JPanel makeFileSelectionPane(String label, 
					     JTextField field,
					     FileDialog fileDialog) 
  {
    class FileSelectionListener implements ActionListener {
      private JTextField field;
      private FileDialog fileDialog;

      public FileSelectionListener(JTextField field, FileDialog fdialog) {
	this.field = field;
	this.fileDialog = fdialog;
      }

      public void actionPerformed(ActionEvent e) {
	fileDialog.show();
	if (fileDialog.getFile() != null && 
	    fileDialog.getFile().length() > 0) {
	  String dir = fileDialog.getDirectory();
	  if (dir.length() > 0) {
	    fileDialog.setDirectory(dir);
	  }
	  field.setText(dir + fileDialog.getFile());
	}
      }
    }

    class FileSelectionPane extends JPanel {
      private JButton browse;
      private JTextField field;
      
      public FileSelectionPane(String label, JTextField field, 
			       FileDialog fdialog) 
      {
	super();
	setLayout(new BorderLayout());
	
	add(new JLabel(label), BorderLayout.WEST);
	add(field, BorderLayout.CENTER);
	this.field = field;

	browse = new JButton("Browse...");
	browse.addActionListener(new FileSelectionListener(field, fdialog));
	add(browse, BorderLayout.EAST);
      }

      public void setEnabled(boolean b) {
	browse.setEnabled(b);
	field.setEnabled(b);
      }
    }

    return new FileSelectionPane(label, field, fileDialog);
  }

  /**
   * Execute the command cmd maybe watching the exit status and reporting
   * errors graphically.
   *
   * @param owner the owner frame of any error dialogs that may appear
   * @param cmd the command to execute
   * @param watchExit whether or not to wait the exit status and report the program's
   * errors
   */
  public static void execute(JFrame owner, String[] cmd, boolean watchExit) {
    class ExecuteThread extends Thread {
      private JFrame owner;
      private String[] cmd;
      private boolean watchExit;

      public ExecuteThread(JFrame owner, String[] cmd, boolean watchExit) {
	this.owner = owner;
	this.cmd = cmd;
	this.watchExit = watchExit;
      }

      public void run() {
	Process p;
	try {
	  p = Runtime.getRuntime().exec(cmd, null);
	} catch (IOException e) {
	  String msg = "An error occurred trying to execute the command:\n";
	  for (int i= 0; i<cmd.length; i++) {
	    msg = msg + cmd[i] + " ";
	  }
	  msg = msg + "\n\nMake sure that you have the program in the correct\n" +
	    "location in the Preferences menu.";
	  JOptionPane.showMessageDialog(owner, msg, "Error", 
					 JOptionPane.ERROR_MESSAGE);
	  return;
	}

	/*
	try {
	  System.err.println("Starting to wait for command: " + cmd[0]);
	  p.waitFor(); // FIXME: icecc sometimes does not terminate ?
	  System.err.println("Ending wait for command: " + cmd[0]);
	} catch (InterruptedException e) { 
	  e.printStackTrace();
	}
	*/

	try {
	  // read in stderr messages because we might want to read about errors
	  BufferedReader reader = new BufferedReader(new InputStreamReader(p.getErrorStream()));
	  Vector messages = new Vector();
	  String line;
	  while ((line = reader.readLine()) != null) {
	    messages.add(line);
	  }

	  // flush stdout so it doesn't block on windows
	  BufferedReader stdout = new BufferedReader(new InputStreamReader(p.getInputStream()));
	  while ((line = stdout.readLine()) != null)
	    ;

	  try {
	    p.waitFor();
	  } catch (InterruptedException e) {
	    e.printStackTrace();
	  }

	  if (watchExit && p.exitValue() != 0)
	    showErrorDialog(owner, messages);
	} catch (IOException e) {
	  e.printStackTrace();
	  /* bad bad bad. oh well */
	}
      }
    }
    
    // If the command is a *.app file and it is a directory, assume that
    // we are in Mac OS X and that the user wants to execute a Carbon App
    // these are executed using "open -a Application.app <files>"
    if (cmd[0].endsWith(".app") && new File(cmd[0]).isDirectory()) {
      String[] newcmd = new String[cmd.length + 2];
      newcmd[0] = "open";
      newcmd[1] = "-a";
      for (int i=0; i<cmd.length; i++)
	newcmd[i+2] = cmd[i];
      cmd = newcmd;
    }

    ExecuteThread e = new ExecuteThread(owner, cmd, watchExit);
    e.start();
  }

  public static String parseErrorMsgForFile(String line) {
    StringTokenizer tok = new StringTokenizer(line, ":");
    // first should be the filename
    String first = tok.hasMoreElements() ? (String)tok.nextElement() : null;
    int count = 0;
    // second should be a parsable line number
    String second = tok.hasMoreElements() ? (String)tok.nextElement() : null;
    try {
      Integer.parseInt(second);
    } catch (NumberFormatException e) {
      // might be windows drive letter... try that
      if (first.length() == 1 && tok.hasMoreElements()) {
	first = first + ":" + second;
	second = (String)tok.nextElement();
	try {
	  Integer.parseInt(second);
	} catch (NumberFormatException e2) {
	  return null;
	}
      } else {
	return null;
      }
    }
    // should be two more elements: error or warning, and the message
    while (tok.hasMoreElements()) {
      tok.nextElement();
      count++;
    }
    if (count == 2)
      return first;
    else
      return null;
  }

}
