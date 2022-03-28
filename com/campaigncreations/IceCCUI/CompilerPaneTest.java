package com.campaigncreations.IceCCUI;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;

public class CompilerPaneTest {

  public static void main(String[] argv) {
    JFrame frame = new JFrame("This is a test");

    Properties prefs = new Properties();
    prefs.setProperty(IceCCUtil.PREF_USE_DEFAULT_ISCRIPT, "true");
    prefs.setProperty(IceCCUtil.PREF_SEPARATE_HEADERS, "false");
    prefs.setProperty(IceCCUtil.PREF_MERGE_DEFAULT_ISCRIPT, "true");
    prefs.setProperty(IceCCUtil.PREF_DISPLAY_ALL_WARNINGS, "true");
    prefs.setProperty(IceCCUtil.PREF_ICECC_EXEC, "../icecc");
    prefs.setProperty(IceCCUtil.PREF_ICEDC_EXEC, "../icedc");
    prefs.setProperty(IceCCUtil.PREF_CONFIG_FILE, "../icecc.ini");
    prefs.setProperty(IceCCUtil.PREF_CONFIG_DIR, "../data");
    prefs.setProperty(IceCCUtil.PREF_TEXT_EDITOR, "emacs");
    IceCCUtil.setPrefs(prefs);
    
    frame.getContentPane().add(new CompilerPane(frame));
    frame.pack();
    frame.setVisible(true);

  }
}
