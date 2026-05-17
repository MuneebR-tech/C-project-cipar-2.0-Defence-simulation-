#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <windows.h>
#include <ctime>
#include <cmath>

using namespace std;

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void setPos(int x, int y) {
    COORD c;
    c.X = (short)(40 + x);
    c.Y = (short)(12 - y);
    SetConsoleCursorPosition(hConsole, c);
}
void setColor(int color) { SetConsoleTextAttribute(hConsole, color); }

struct Target {
    int id;
    float x, y;
    bool isHostile;
    bool destroyed;
    char symbol;
    int risk;
    void evaluate() {
        float dist = sqrt(x * x + y * y);
        risk = (isHostile ? 55 : 0) + (int)(120 / (dist + 1));
    }
};

// ---------------- BST ----------------
class Node {
public:
    Target data;
    Node* left;
    Node* right;
    Node(Target t) { data = t; left = NULL; right = NULL; }
};

class BST {
public:
    Node* root;
    BST() { root = NULL; }

    Node* insert(Node* node, Target t) {
        if (!node) return new Node(t);
        if (t.risk < node->data.risk) node->left = insert(node->left, t);
        else node->right = insert(node->right, t);
        return node;
    }

    void insert(Target t) { root = insert(root, t); }

    Node* findPriority(Node* node, char sym) {
        if (!node) return NULL;
        if (node->data.isHostile && !node->data.destroyed && node->data.symbol == sym) return node;
        Node* left = findPriority(node->left, sym);
        if (left) return left;
        return findPriority(node->right, sym);
    }

    Node* findPriority(char sym) { return findPriority(root, sym); }

    void clear(Node* node) {
        if (!node) return;
        clear(node->left); clear(node->right);
        delete node;
    }
    void clear() { clear(root); root = NULL; }
};

// ---------------- SiparSystem ----------------
class SiparSystem {
private:
    vector<Target> allTargets;   // persistent battlefield
    vector<string> history;      // logs
    queue<Target> waveQueue;     // incoming waves
    BST activeHostiles;          // BST of hostiles
    char missileBar[20];         // ammo bar (array)
    int missiles;
    int breaches;
    int crosshairX, crosshairY;  // Manual targeting crosshair
    bool manualMode;             // Toggle for manual/auto targeting

    void boomAnimation(Target& t) {
        int tx = (int)t.x;
        int ty = (int)t.y / 2;
        char frames[4] = { '*', '#', '@', '*' };
        int colors[4] = { 14, 12, 6, 14 };
        for (int i = 0; i < 4; i++) {
            setPos(tx, ty);
            setColor(colors[i]);
            cout << frames[i];
            Sleep(150);
            setPos(tx, ty);
            cout << " ";
        }
        setPos(tx, ty);
        setColor(8);
        cout << "N";
    }

public:
    SiparSystem() {
        srand((unsigned)time(0));
        missiles = 15;
        breaches = 0;
        crosshairX = 0;
        crosshairY = 0;
        manualMode = false;  // Start with auto-targeting
        for (int i = 0; i < 20; i++) missileBar[i] = '|';
    }

    void initWave() {
        // preload queue with targets
        Target t1{ 301, 35, 10, true, false, 'X' }; t1.evaluate();
        Target t2{ 302, 40, -8, true, false, '.' }; t2.evaluate();
        Target t3{ 303, 25, 6, true, false, 'X' }; t3.evaluate();
        Target t4{ 304, 30, -12, true, false, '.' }; t4.evaluate();
        Target t5{ 305, 0, -20, false, false, 'O' }; t5.evaluate();
        Target t6{ 306, 45, 5, true, false, 'X' }; t6.evaluate();
        Target t7{ 307, 50, -6, true, false, '.' }; t7.evaluate();
        waveQueue.push(t1); waveQueue.push(t2);
        waveQueue.push(t3); waveQueue.push(t4);
        waveQueue.push(t5); waveQueue.push(t6);
        waveQueue.push(t7);
    }

    int countHostiles() {
        int count = 0;
        for (auto& t : allTargets) {
            if (t.isHostile && !t.destroyed) count++;
        }
        return count;
    }

    void deployWave() {
        if (!waveQueue.empty() && countHostiles() < 6) {
            Target t = waveQueue.front(); waveQueue.pop();
            allTargets.push_back(t);
        }
    }

    void rebuildBST() {
        activeHostiles.clear();
        for (auto& t : allTargets) {
            if (t.isHostile && !t.destroyed) {
                activeHostiles.insert(t);
            }
        }
    }

    void updatePositions() {
        for (auto& t : allTargets) {
            if (!t.destroyed) {
                if (t.isHostile) t.x -= 1;
                else if (t.symbol == 'O') t.y += (rand() % 3 - 1);
            }
        }
    }

    void draw() {
        for (auto& t : allTargets) {
            setPos((int)t.x, (int)t.y / 2);
            if (t.destroyed) { setColor(8); cout << "N"; }
            else if (t.isHostile) { setColor(12); cout << t.symbol; }
            else { setColor(10); cout << "<O>"; }
        }
    }

    void drawCrosshair() {
        if (manualMode) {
            setColor(14);  // Yellow crosshair
            // Draw horizontal line
            for (int i = -2; i <= 2; i++) {
                if (i != 0) {
                    setPos(crosshairX + i, crosshairY);
                    cout << "-";
                }
            }
            // Draw vertical line
            for (int i = -1; i <= 1; i++) {
                if (i != 0) {
                    setPos(crosshairX, crosshairY + i);
                    cout << "|";
                }
            }
            // Draw center
            setPos(crosshairX, crosshairY);
            setColor(10);
            cout << "+";
        }
    }

    void moveCrosshair(int dx, int dy) {
        crosshairX += dx;
        crosshairY += dy;
        // Keep crosshair within bounds
        if (crosshairX < -39) crosshairX = -39;
        if (crosshairX > 39) crosshairX = 39;
        if (crosshairY < -12) crosshairY = -12;
        if (crosshairY > 12) crosshairY = 12;
    }

    void toggleManualMode() {
        manualMode = !manualMode;
        if (manualMode) {
            history.push_back("Switched to MANUAL targeting");
        }
        else {
            history.push_back("Switched to AUTO targeting");
        }
    }

    Target* findTargetAtCrosshair() {
        for (auto& t : allTargets) {
            if (!t.destroyed) {
                int tx = (int)t.x;
                int ty = (int)t.y / 2;
                if (tx == crosshairX && ty == crosshairY) {
                    return &t;
                }
            }
        }
        return NULL;
    }

    void fire() {
        if (missiles <= 0) {
            setPos(-20, -12); setColor(12);
            cout << "Not enough stock!";
            Sleep(700);
            return;
        }

        missiles--;
        missileBar[missiles] = ' '; // update ammo bar

        if (manualMode) {
            // Manual targeting
            Target* t = findTargetAtCrosshair();
            if (t) {
                if (t->isHostile) {
                    setPos(-25, -12); setColor(14); cout << "[MANUAL TARGETING] → LOCKED ON"; Sleep(400);
                    setPos(-25, -11); cout << "[MISSILE LAUNCH] → IMPACT CONFIRMED"; Sleep(400);
                    setPos(-25, -10); cout << "[TARGET DESTROYED]"; Sleep(400);

                    boomAnimation(*t);
                    t->isHostile = false;
                    t->destroyed = true;
                    history.push_back("Manual kill: " + to_string(t->id));
                }
                else {
                    setPos(-20, -12); setColor(11);
                    cout << "You targeted non-hostile target <O>";
                    history.push_back("Friendly fire!");
                    Sleep(700);
                }
            }
            else {
                setPos(-20, -12); setColor(8);
                cout << "Missile missed! No target at crosshair";
                history.push_back("Missile missed");
                Sleep(700);
            }
        }
        else {
            // Auto targeting
            Node* threat = activeHostiles.findPriority('X');
            if (!threat) threat = activeHostiles.findPriority('.');

            if (threat) {
                setPos(-25, -12); setColor(14); cout << "[AUTO-TARGETING] → LOCKED ON"; Sleep(400);
                setPos(-25, -11); cout << "[MISSILE LAUNCH] → IMPACT CONFIRMED"; Sleep(400);
                setPos(-25, -10); cout << "[TARGET DESTROYED]"; Sleep(400);

                for (auto& t : allTargets) {
                    if (t.id == threat->data.id) {
                        boomAnimation(t);
                        t.isHostile = false;
                        t.destroyed = true;
                        history.push_back("Auto kill: " + to_string(t.id));
                        break;
                    }
                }
            }
            else {
                setPos(-20, -12); setColor(11);
                cout << "You targeted non-hostile target <O>";
                history.push_back("Friendly fire!");
                Sleep(700);
            }
        }
    }

    void monitorBase() {
        for (auto& t : allTargets) {
            if (t.isHostile && !t.destroyed && t.x <= -40) {
                setPos(-25, -12); setColor(9);
                if (t.symbol == 'X') cout << "Enemy missile hit our base!";
                else if (t.symbol == '.') cout << "A hostile drone entered our base!";
                Sleep(700);
                t.isHostile = false; t.destroyed = true;
                breaches++;
                history.push_back("Base breached!");
            }
        }
    }

    void printHistory() {
        setPos(-38, 10); setColor(7);
        cout << "LOG: ";
        for (auto& h : history) cout << h << " | ";
    }

    void printMissiles() {
        setPos(30, -12); setColor(11);
        cout << "Missiles: " << missiles << " ";
        cout << "[";
        for (int i = 0; i < 20; i++) cout << missileBar[i];
        cout << "]";
    }

    void drawBaseLine() {
        setColor(9);
        for (int j = -12; j <= 12; j++) {
            setPos(-40, j);
            cout << "|";
        }
    }

    void printManual() {
        setPos(-38, -14);
        setColor(12); cout << "X=Missile  ";
        setColor(11); cout << ".=Drone  ";
        setColor(10); cout << "<O>=Friendly  ";
        setColor(14); cout << "HQ=Base  ";
        setColor(9);  cout << "Line=Defense  ";
        setColor(8);  cout << "N=Neutralized";
    }

    void printControls() {
        setPos(-38, -16);
        setColor(13);  // Magenta for controls
        cout << "CONTROLS: [SPACE]=Fire  [M]=Manual/Auto  [WASD]=Move Crosshair  [E]=Exit";
    }

    int getNeutralizedCount() {
        int count = 0;
        for (auto& t : allTargets) if (t.destroyed) count++;
        return count;
    }

    int getBreaches() { return breaches; }
    int getRemainingMissiles() { return missiles; }
    bool isManualMode() { return manualMode; }

}; // <-- close SiparSystem class here

// ---------------- MAIN ----------------
void displayTitleScreen() {
    // Set dark grey background
    system("color 80");
    system("cls");

    // Title with smooth appearance
    for (int i = 0; i < 3; i++) {
        system("cls");
        setColor(7 + i);  // Gradually brighter
        cout << "\n\n\n";
        cout << "\t    ╔══════════════════════════════════════════╗\n";
        cout << "\t    ║         SIPAR DEFENSE SYSTEM             ║\n";
        cout << "\t    ║      REMOTE TARGETING COMMAND            ║\n";
        cout << "\t    ╚══════════════════════════════════════════╝\n\n";
        Sleep(300);
    }

    setColor(15);  // Bright white
    cout << "\n\t       [ A Next-Generation Battlefield Management ]\n\n";

    setColor(8);  // Grey
    cout << "\t    ───────────────────────────────────────────\n";

    setColor(7);  // Regular grey
    cout << "\n\t      • Real-time threat assessment\n";
    cout << "\t      • BST-based priority targeting\n";
    cout << "\t      • Manual/Auto fire control\n";
    cout << "\t      • Base defense simulation\n\n";

    setColor(14);  // Yellow
    cout << "\t      Press A to LAUNCH SYSTEM\n";
    cout << "\t      Press E to EXIT\n\n";

    setColor(8);  // Dark grey
    cout << "\t    ───────────────────────────────────────────\n";

    // Smooth transition to normal colors - FIXED VERSION
    Sleep(500);
    // Create color strings manually
    string colors[] = { "85", "84", "83", "82", "81", "80", "07" };
    for (int i = 0; i < 7; i++) {
        string cmd = "color " + colors[i];
        system(cmd.c_str());  // Convert to C-style string
        Sleep(100);
    }
}

int main() {
    displayTitleScreen();

    char command;
    cin >> command;

    if (command == 'E' || command == 'e') {
        system("color 07");
        return 0;
    }
    if (!(command == 'A' || command == 'a')) {
        system("color 07");
        return 0;
    }

    // Final transition to normal colors
    system("color 07");

    Sleep(1000);

    SiparSystem sipar;
    int frame = 0;

    sipar.initWave();

    while (frame < 80) {
        system("cls");
        sipar.rebuildBST();   // keep BST in sync

        setColor(11);
        cout << "------------------------------------------------------------\n";
        cout << " SIPAR RADAR ENGINE | PULSE: " << frame;
        cout << " | MODE: " << (sipar.isManualMode() ? "MANUAL [+]" : "AUTO   ");
        cout << "\n------------------------------------------------------------\n";

        setPos(0, 0); setColor(14); cout << "[HQ]";

        if (frame % 10 == 0) sipar.deployWave();

        sipar.updatePositions();
        sipar.draw();
        sipar.drawCrosshair();  // Draw crosshair if in manual mode
        sipar.printHistory();
        sipar.printMissiles();
        sipar.drawBaseLine();
        sipar.printManual();
        sipar.printControls();
        sipar.monitorBase();

        // Handle controls
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            sipar.fire();
            Sleep(300);  // Prevent rapid firing
        }
        if (GetAsyncKeyState('M') & 0x8000) {
            sipar.toggleManualMode();
            Sleep(300);  // Prevent rapid toggling
        }
        if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000) {
            sipar.moveCrosshair(0, 1);
        }
        if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000) {
            sipar.moveCrosshair(0, -1);
        }
        if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
            sipar.moveCrosshair(-1, 0);
        }
        if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) {
            sipar.moveCrosshair(1, 0);
        }
        if (GetAsyncKeyState('E') & 0x8000) break;

        setPos(-38, -10); setColor(8);
        if (sipar.isManualMode()) {
            cout << ">> MANUAL: Move crosshair [WASD], Fire [SPACE], Switch [M]";
        }
        else {
            cout << ">> AUTO: Fire [SPACE] to auto-target, Switch to manual [M]";
        }

        Sleep(300);
        frame++;
    }

    system("cls");
    setColor(11);
    cout << "\n===== DEFENSE SUMMARY =====\n";
    cout << "Missiles Remaining: " << sipar.getRemainingMissiles() << endl;
    cout << "Hostiles Neutralized: " << sipar.getNeutralizedCount() << endl;
    cout << "Breaches: " << sipar.getBreaches() << endl;
    cout << "===========================\n\n";

    if (sipar.getBreaches() == 0) {
        setColor(10);
        cout << "MISSION SUCCESSFUL: Base Secured\n";
    }
    else {
        setColor(12);
        cout << "MISSION FAILED: Base Compromised\n";
    }

    return 0;
}