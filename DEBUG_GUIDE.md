# 🐛 Visual Studio Debugging Guide

## 🎯 **Steps to Debug the Crash**

### **1. Visual Studio Should Now Be Open**

The project `voip-client.vcxproj` should be loaded.

### **2. Set Startup Project**

- In **Solution Explorer**, right-click on **voip-client** project
- Click **"Set as Startup Project"**

### **3. Configure Debugging**

- Right-click **voip-client** → **Properties**
- Go to **Debugging**
- Make sure:
  - **Working Directory**: `$(ProjectDir)\Debug` or `.`
  - **Debugger Type**: **Auto** or **Mixed (Managed and Native)**

### **4. Enable All Exceptions**

This will catch the crash immediately:

- Go to **Debug** menu → **Windows** → **Exception Settings** (or press `Ctrl+Alt+E`)
- Check the box next to **C++ Exceptions** to break on ALL C++ exceptions
- Also check **Win32 Exceptions**

### **5. Start Debugging**

Press **F5** or click **Debug → Start Debugging**

### **6. What to Look For**

When it crashes, Visual Studio will pause and show:

**A. The Call Stack:**
- Look at the **Call Stack** window (Debug → Windows → Call Stack)
- Find which function is crashing
- Look for our code (voip::...) not just Qt code

**B. The Exception:**
- The Output window will show the exception message
- Look for exception codes:
  - `0x80000003` = Debug assertion / breakpoint
  - `0xc0000005` = Access violation (null pointer)
  - `0xc00000fd` = Stack overflow

**C. Local Variables:**
- Check **Locals** window to see what values caused the crash
- See if any pointers are null

### **7. Common Issues to Check**

**If crash is in Qt code:**
- DLL version mismatch (vcpkg vs Qt installation)
- Missing Qt plugin

**If crash is in our code:**
- Constructor issue (HotkeyManager or ChannelWidget)
- Stack overflow from recursive calls
- Null pointer dereference

### **8. Report Back:**

Please note:
1. **Which function is at the top of the call stack?**
2. **What is the exception message/code?**
3. **Does the debugger stop before "=== VoIP Client Starting ==="?**

---

## 🔧 **Alternative: Quick Test Without Multi-Channel**

If debugging is too slow, I can create a minimal version without the new multi-channel code to verify the base client works.

---

## 📊 **Expected Debug Output**

**If working correctly, you should see:**
```
=== VoIP Client Starting ===
Creating QApplication...
QApplication created successfully!
Application info set
About to create LoginDialog...
LoginDialog created!
```

**If crashing before any output:**
- The crash is in static initialization OR
- Qt DLL loading failure OR
- First line of main() is crashing

---

## 🚨 **Quick Fixes to Try**

### **Fix 1: Disable Debug Iterators**

Add to the top of `ui_main.cpp`:
```cpp
#define _ITERATOR_DEBUG_LEVEL 0
```

### **Fix 2: Check DLL Versions**

In Visual Studio Output window during debugging, look for:
```
Loaded 'C:\...\Qt6Cored.dll'
```

Make sure all Qt DLLs are from the same version (6.10.1).

### **Fix 3: Disable Our New Code Temporarily**

Comment out HotkeyManager and ChannelWidget creation in MainWindow to see if they're causing the crash.

---

## ✅ **When You Find the Crash**

Take a screenshot of:
1. The Call Stack window
2. The Exception message
3. The line of code where it crashed

Then I can provide a targeted fix!
