# 📚 Assistant No.2 Practice Tool

[简体中文](README.md)

A high-performance, cross-platform question practice software developed with **C++23** and **Qt 6** (Widgets).

**Core Purpose:** Designed specifically for reviewing quizzes, homework, and exams from platforms like **Chaoxing (Learning Toolkit)**. It works offline, supports **custom parsing strategies**, features **highly configurable practice policies**, and runs perfectly on both **Windows** and **Android**.

![screenshot](./.img/1_0.png)

## ✨ Features

### 🧠 Powerful Parser Engine

*   **Full Format Support**: Perfectly parses **Single Choice, Multiple Choice, True/False, and Fill-in-the-blank**.
*   **Smart Text Parsing**:
    *   **Zero Regex Dependency**: Uses a hand-written high-performance parser that supports **UTF-8 full-width punctuation** (e.g., `．` `（` `）` `：`), completely solving encoding and truncation issues.
    *   **Inline Answer Extraction**: Extracts answers embedded within the question text (e.g., `...is (A)...`).
    *   **Configurable Strategies**: Customize "Start Keywords", "Answer Keywords", etc., to adapt to different question bank sources.
*   **Multi-Dimension Modes**:
    *   **Free Practice**: Auto-advances on correct answers; highlights mistakes.
    *   **Review Mode**: Directly displays answers for quick memorization.
    *   **Mistake Book**: Automatically records errors, supporting focused review by "Error Count ≥ N" or "= N".

### 🎓 Exam Simulation System

*   **Real Simulation**: Supports **countdown timer** to simulate real exam pressure.
*   **Custom Configuration**: Freely set **number of questions** and **score weight** for each question type.
*   **Answer Sheet**: Visual answer sheet status (Answered/Unanswered) with one-click navigation.
*   **Score Statistics**: Generates detailed reports after submission, verifying total score, accuracy, and time used.

### 🛡️ Advanced Practice Strategies

*   **Smart De-duplication**: Removes duplicate questions when importing multiple files.
*   **Junk Filter**:
    *   **Skip Common Answers**: One-click skip for filler questions where the answer is always "A" or "True".
    *   **Exclude Multi-Select All**: Automatically ignores "All of the above" type questions.
    *   **Skip Specific Options**: Manually skip questions with specific answers (e.g., "C").

### 📱 Mobile/UI Optimization

*   **Native Dark Mode**: Global dark theme based on Qt Palette, protecting your eyes at night.
*   **Silky Layout**: Adhering to minimalist design philosophy, optimizing component hierarchy and dynamic loading logic to provide a smooth experience that follows your touch.
*   **Adaptive Layout**: Font size and control spacing automatically adapt to different screen DPIs.

## 🚀 Quick Start

### 1. Run App

*   **Windows**: Download [helper02.exe](https://github.com/Iviesever/Helper-02/releases/latest/download/helper02.exe) directly.
*   **Android**: Download [helper02.apk](https://github.com/Iviesever/Helper-02/releases/latest/download/helper02.apk) directly and install.

### 2. 📂 How to Create Question Bank (Core)

The most powerful feature of this software is the "What You See Is What You Get" text import.

1.  **Get Question Text**:
    *   Open the **Homework**, **Quiz**, or **Exam Review** page on the learning platform in a PC browser.
    *   **Select All**: Use the shortcut **`Ctrl + A`**.
    *   **Copy**: Use **`Ctrl + C`** to copy all selected text.
2.  **Save File**:
    *   Create a new `.txt` file.
    *   Paste the content.
    *   **⚠️ Critical**: Save as **UTF-8** encoding (to prevent character encoding issues).
3.  **Import**:
    *   Place the `.txt` file in the recognized directory, or select it directly on the app home page.

### 3. Data Storage

- Storage paths can be specified in settings.
- You can find storage examples in `example`:

```cpp
├── QuestionBank/ 
    ├── Marxism/
    │   ├── Chapter1_Quiz.txt
    │   └── Final_Sim.txt
    └── ComputerBasics/
        └── Bank1.txt
```

```cpp
├── Data/
    ├── config.json              // Global config (Theme, Font, Last State)
    ├── exam_configs.json        // Exam mode presets
    ├── history.json             // Exam score records
    ├── mistakes.json            // Mistake database (Stores Question ID and Error Count)
    ├── parser_strategies.json   // Custom text parsing strategies
    └── practice_strategies.json // Practice filter strategies
```

## ⚙️ Tips

- **Parser Strategy**: If the imported questions are not recognized correctly (e.g., answer is labeled "My Answer" in one file and "Standard Answer" in another), add corresponding keywords in "Parser Strategy Management".
- **Combined Practice**: Hold `Ctrl` or use checkboxes to select multiple files on the home page; the system will automatically merge questions.
- **State Memory**: The software automatically remembers your last progress, selected files, and active strategies.

## 📝 License

This project is licensed under the **MIT License**.

This means you are free to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the software, provided the original author's copyright notice and permission notice are included.

------

### The MIT License (MIT)

Copyright (c) 2026 Iviesever

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------

#### **Note on Qt Static Linking Compliance**

This project (.exe) uses the open-source version of the Qt 6 framework (LGPLv3) and is **statically linked**.
According to the LGPLv3 license requirements, the author has provided the **object files archive** of this program at GitHub **[object_files.zip](https://github.com/Iviesever/Helper-02/blob/main/object_files.zip)**.
If you wish to re-link this program using a different version of the Qt library, please download the archive and use your linker to proceed.

#### **Legal Disclaimer**

1.  **Educational Use Only**: This project and its related code and documentation are intended solely for learning, research, and technical exchange.
2.  **No Warranty**: This project is provided "AS IS", without any warranty of any kind, express or implied, including but not limited to warranties of merchantability, fitness for a particular purpose, or non-infringement. Users assume all risks associated with the use of this software.
3.  **Compliance Notice**: The information regarding Qt open source licensing mentioned above is merely a **friendly reminder** based on the author's personal understanding and **does not constitute legal advice**. The author is not a legal expert and cannot guarantee the accuracy or completeness of this interpretation. Before using, distributing, or modifying this program for commercial purposes, please consult professional legal counsel or read the [Qt LGPLv3 License](https://www.gnu.org/licenses/lgpl-3.0.html) in detail.
4.  **Limitation of Liability**: To the maximum extent permitted by law, the author shall **not be liable** for any direct, indirect, incidental, special, or consequential damages (including but not limited to data loss, device failure, business interruption, or legal disputes) arising from the use or inability to use this software (or its derivatives).
