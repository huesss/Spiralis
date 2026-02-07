#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include <sstream>
#include <vector>
#include <random>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
void move_cursor_home() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0, 0};
    SetConsoleCursorPosition(hOut, pos);
}
void hide_cursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
}
void clear_screen() {
    system("cls");
}
void get_terminal_size(int& width, int& height) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}
#else
void move_cursor_home() {
    std::cout << "\033[H";
}
void hide_cursor() {
    std::cout << "\033[?25l";
}
void clear_screen() {
    std::cout << "\033[2J\033[H";
}
void get_terminal_size(int& width, int& height) {
    width = 120;
    height = 40;
}
#endif

using namespace std;

namespace {
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = 2.0 * PI;
    
    const string GRADIENT = " .:-=+*#%@";
    
    mt19937 rng(42);
    
    double random_double(double min_val, double max_val) {
        uniform_real_distribution<double> dist(min_val, max_val);
        return dist(rng);
    }
    
    struct Vec2 {
        double x, y;
        
        Vec2(double x_ = 0, double y_ = 0) : x(x_), y(y_) {}
        
        Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
        Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
        Vec2 operator*(double s) const { return {x * s, y * s}; }
        
        double length() const { return sqrt(x * x + y * y); }
        Vec2 normalized() const { 
            double len = length();
            return len > 0 ? Vec2{x / len, y / len} : Vec2{0, 0};
        }
        Vec2 perpendicular() const { return {-y, x}; }
    };
    
    struct Particle {
        Vec2 pos;
        double angle;
        double radius;
        double angular_velocity;
        double brightness;
        
        Particle(double r, double a, double av, double b)
            : radius(r), angle(a), angular_velocity(av), brightness(b) {}
        
        void update(double dt) {
            angle += angular_velocity * dt;
            if (angle > TWO_PI) angle -= TWO_PI;
            if (angle < 0) angle += TWO_PI;
        }
        
        Vec2 get_position(const Vec2& center, double aspect) const {
            return {
                center.x + radius * cos(angle) * aspect,
                center.y + radius * sin(angle)
            };
        }
    };
    
    struct Star {
        Vec2 pos;
        double phase;
        double speed;
        double base_brightness;
        
        void update(double dt) {
            phase += speed * dt;
            if (phase > TWO_PI) phase -= TWO_PI;
        }
        
        double get_brightness() const {
            return base_brightness * (0.3 + 0.7 * (0.5 + 0.5 * sin(phase)));
        }
    };
    
    class Galaxy {
    private:
        vector<Particle> particles_;
        vector<Star> stars_;
        Vec2 center_;
        int width_, height_;
        double time_;
        double aspect_ratio_;
        
    public:
        Galaxy(int w, int h) : width_(w), height_(h), time_(0) {
            center_ = {w / 2.0, h / 2.0};
            aspect_ratio_ = 2.0;
            
            init_spiral_arms();
            init_core();
            init_background_stars();
        }
        
        void update(double dt) {
            time_ += dt;
            
            for (auto& p : particles_) {
                p.update(dt);
            }
            
            for (auto& s : stars_) {
                s.update(dt);
            }
        }
        
        void render(double real_elapsed_sec = 0) const {
            vector<string> screen(height_, string(width_, ' '));
            vector<vector<double>> intensity(height_, vector<double>(width_, 0));
            
            render_stars(screen);
            accumulate_particles(intensity);
            apply_intensity(screen, intensity);
            render_core(screen);
            
            output(screen, real_elapsed_sec);
        }
        
    private:
        void init_spiral_arms() {
            constexpr int num_arms = 2;
            constexpr int particles_per_arm = 150;
            
            for (int arm = 0; arm < num_arms; ++arm) {
                double arm_offset = arm * PI;
                
                for (int i = 0; i < particles_per_arm; ++i) {
                    double t = i / static_cast<double>(particles_per_arm);
                    double base_radius = 2.0 + t * 14.0;
                    double spiral_angle = arm_offset + t * 2.5 * PI;
                    
                    double radius_variation = random_double(-1.0, 1.0) * (0.5 + t * 1.5);
                    double angle_variation = random_double(-0.2, 0.2);
                    
                    double radius = base_radius + radius_variation;
                    double angle = spiral_angle + angle_variation;
                    
                    double angular_velocity = 0.15 / sqrt(radius);
                    double brightness = 0.3 + 0.7 * (1.0 - t * 0.6);
                    
                    particles_.emplace_back(radius, angle, angular_velocity, brightness);
                }
            }
        }
        
        void init_core() {
            constexpr int core_particles = 60;
            
            for (int i = 0; i < core_particles; ++i) {
                double radius = random_double(0.5, 3.0);
                double angle = random_double(0, TWO_PI);
                double angular_velocity = 0.3 / sqrt(radius + 0.5);
                double brightness = 0.8 + random_double(0, 0.2);
                
                particles_.emplace_back(radius, angle, angular_velocity, brightness);
            }
        }
        
        void init_background_stars() {
            constexpr int num_stars = 80;
            
            for (int i = 0; i < num_stars; ++i) {
                Star s;
                s.pos = {random_double(0, width_), random_double(0, height_)};
                s.phase = random_double(0, TWO_PI);
                s.speed = random_double(0.5, 2.0);
                s.base_brightness = random_double(0.3, 1.0);
                stars_.push_back(s);
            }
        }
        
        void render_stars(vector<string>& screen) const {
            for (const auto& s : stars_) {
                int sx = static_cast<int>(s.pos.x);
                int sy = static_cast<int>(s.pos.y);
                
                if (sx >= 0 && sx < width_ && sy >= 0 && sy < height_) {
                    double b = s.get_brightness();
                    if (b > 0.7) screen[sy][sx] = '*';
                    else if (b > 0.4) screen[sy][sx] = '+';
                    else if (b > 0.2) screen[sy][sx] = '.';
                }
            }
        }
        
        void accumulate_particles(vector<vector<double>>& intensity) const {
            for (const auto& p : particles_) {
                Vec2 pos = p.get_position(center_, aspect_ratio_);
                int px = static_cast<int>(pos.x);
                int py = static_cast<int>(pos.y);
                
                if (px >= 0 && px < width_ && py >= 0 && py < height_) {
                    intensity[py][px] += p.brightness;
                }
            }
        }
        
        void apply_intensity(vector<string>& screen, const vector<vector<double>>& intensity) const {
            for (int y = 0; y < height_; ++y) {
                for (int x = 0; x < width_; ++x) {
                    if (intensity[y][x] > 0.1) {
                        int idx = static_cast<int>(intensity[y][x] * 3.0);
                        idx = clamp(idx, 0, static_cast<int>(GRADIENT.length()) - 1);
                        screen[y][x] = GRADIENT[idx];
                    }
                }
            }
        }
        
        void render_core(vector<string>& screen) const {
            int cx = static_cast<int>(center_.x);
            int cy = static_cast<int>(center_.y);
            
            if (cx > 0 && cx < width_ - 1 && cy >= 0 && cy < height_) {
                screen[cy][cx] = '@';
                screen[cy][cx - 1] = '(';
                screen[cy][cx + 1] = ')';
            }
        }
        
        void output(const vector<string>& screen, double real_elapsed_sec) const {
            move_cursor_home();
            ostringstream buffer;
            
            for (const auto& line : screen) {
                buffer << line << '\n';
            }
            
            buffer << "\n Time: " << static_cast<int>(real_elapsed_sec) << "s";
            cout << buffer.str();
            cout.flush();
        }
    };
}

int main() {
    hide_cursor();
    clear_screen();
    
    int term_width, term_height;
    get_terminal_size(term_width, term_height);
    
    int width = min(term_width, 120);
    int height = min(term_height - 3, 35);
    
    Galaxy galaxy(width, height);
    
    constexpr double dt = 0.1;
    constexpr auto frame_duration = chrono::milliseconds(50);
    auto start_time = chrono::steady_clock::now();
    
    while (true) {
        auto now = chrono::steady_clock::now();
        double real_elapsed = chrono::duration<double>(now - start_time).count();
        galaxy.render(real_elapsed);
        galaxy.update(dt);
        this_thread::sleep_for(frame_duration);
    }

    return 0;
}