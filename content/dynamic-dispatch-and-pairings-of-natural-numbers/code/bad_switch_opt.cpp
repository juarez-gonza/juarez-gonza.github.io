#include <cstdint>

extern int a();
extern int b();
extern int c();
extern int d();
extern int e();
extern int f();
extern int g();
extern int h();
extern int i();
extern int j();
extern int k();
extern int l();
extern int handle_error();

enum class states : std::uint16_t { A, B, C, D };
enum class inputs : std::uint32_t { a, b, c };

int dispatch_nested(inputs x, states y) {
    switch (x) {
        case inputs::a:
            switch (y) {
                case states::A:
                    return a();
                case states::B:
                    return b();
                case states::C:
                    return c();
                case states::D:
                    return d();
                default:
                    return handle_error();
            }

        case inputs::b:
            switch (y) {
                case states::A:
                    return f();
                case states::B:
                    return g();
                case states::C:
                    return h();
                case states::D:
                    return i();
                default:
                    return handle_error();
            }

        case inputs::c:
            switch (y) {
                case states::A:
                    return k();
                case states::B:
                    return l();
                case states::C:
                    return a();
                case states::D:
                    return g();
                default:
                    return handle_error();
            }

        default:
            return handle_error();
    }
}

int dispatch_flatten(inputs x, states y) {
    switch (static_cast<std::uint32_t>(x) * 4 + static_cast<std::uint16_t>(y)) {
        case 0:  // x=0, y=0
            return a();
        case 1:  // x=0, y=1
            return b();
        case 2:  // x=0, y=2
            return c();
        case 3:  // x=0, y=3
            return d();
        case 4:  // x=1, y=0
            return f();
        case 5:  // x=1, y=1
            return g();
        case 6:  // x=1, y=2
            return h();
        case 7:  // x=1, y=3
            return i();
        case 8:  // x=2, y=0
            return k();
        case 9:  // x=2, y=1
            return l();
        case 10:  // x=2, y=2
            return a();
        case 11:  // x=2, y=3
            return g();
        default:
            // Covers:
            //   x=0, y>=4
            //   x=1, y>=4
            //   x=2, y>=4
            //   x!=0,1,2
            return handle_error();
    }
}
