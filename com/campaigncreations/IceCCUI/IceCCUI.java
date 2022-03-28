/*
 * IceCCUI
 *
 * Author: Jeff Pang <jp@magnus99.dhs.org>
 *
 * CVS: $Id: IceCCUI.java,v 1.2 2002/06/08 10:12:41 jp Exp $
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
 * This is the main IceCC gui.
 */
public class IceCCUI extends JFrame implements ActionListener {

  public static final String INFO =
    "IceCCUI. Graphical user interface component for IceCC\n" +
    "Copyright (C) 2001 Jeffrey Pang <jp@magnus99.dhs.org>\n\n" +
    "This program is free software; you can redistribute it and/or\n" +
    "modify it under the terms of the GNU General Public License\n" +
    "as published by the Free Software Foundation; either version 2\n" +
    "of the License, or (at your option) any later version.\n\n" +
    "This program is distributed in the hope that it will be useful\n" +
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" +
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" +
    "GNU General Public License for more details.\n\n" +
    "You should have received a copy of the GNU General Public License\n" +
    "along with this program; if not, write to the Free Software\n" +
    "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.";

  // reference to the root frame
  protected static JFrame rootFrame;

  private JMenuItem prefs, info, quit;

  // should be a singleton
  private IceCCUI() {
    super("IceCC UI - Starcraft Iscript.bin Editor");

    rootFrame = this;

    JTabbedPane mainPane = new JTabbedPane();
    mainPane.addTab("Decompiler", new DecompilerPane(this));
    mainPane.addTab("Compiler", new CompilerPane(this));

    getContentPane().add(mainPane);

    JMenuBar menuBar = new JMenuBar();
    JMenu file = new JMenu("File");
    file.setMnemonic(KeyEvent.VK_F);
    prefs = new JMenuItem("Preferences", KeyEvent.VK_P);
    prefs.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_P, ActionEvent.CTRL_MASK));
    prefs.addActionListener(this);
    info = new JMenuItem("Info", KeyEvent.VK_I);
    info.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_I, ActionEvent.CTRL_MASK));
    info.addActionListener(this);
    quit = new JMenuItem("Quit", KeyEvent.VK_Q);
    quit.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Q, ActionEvent.CTRL_MASK));
    quit.addActionListener(this);

    file.add(prefs);
    file.add(info);
    file.addSeparator();
    file.add(quit);
    menuBar.add(file);

    setJMenuBar(menuBar);
    pack();
  }

  public void actionPerformed(ActionEvent e) {
    Object source = e.getSource();

    if (source == prefs) {
      (new PreferencesDialog(this)).setVisible(true);
      return;
    } else if (source == info) {
      JOptionPane.showMessageDialog(this, INFO, "IceCC by Jeff Pang aka DI",
				    JOptionPane.INFORMATION_MESSAGE);
    } else if (source == quit) {
      IceCCUtil.savePrefs();
      dispose();
      System.exit(0);
    }
  }

  public static void main(String[] args) {
    IceCCUtil.loadPrefs();
    IceCCUI instance = new IceCCUI();
    instance.addWindowListener(new WindowAdapter() {
	public void windowClosing(WindowEvent e) {
	  IceCCUtil.savePrefs();
	  System.exit(0);
	}
      });
    instance.setVisible(true);
  }
}
