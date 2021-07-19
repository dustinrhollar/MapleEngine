#ifndef _UTILS_MAPLE_MATH_H
#define _UTILS_MAPLE_MATH_H

// Disable warnings about unnamed structs/unions
#pragma warning(disable : 4201)

#ifndef MM_PI
#define MM_PI 3.141592653589793238f 
#endif

typedef union
{
    struct { i32 x, y; };
    i32 p[2];
} iv2;

typedef union
{
    struct { i32 x, y, z; };
    struct { i32 xy, p0; };
    struct { i32 p1, yz; };
    i32 p[3];
} iv3;


typedef union
{
    r32 p[2];
    struct { r32 x, y; };
} Vec2;

typedef union
{
    r32 p[3];
    struct { r32 x, y, z; };
    struct { Vec2 xy; r32 p0; };
    struct { r32 p1; Vec2 yz; };
    
    struct { r32 r, g, b; };
    struct { Vec2 rg; r32 p2; };
    struct { r32 p3; Vec2 gb; };
} Vec3;

typedef union
{
    r32 p[4];
    struct { r32 x, y, z, w; };
    struct { Vec2 xy; Vec2 zw; };
    struct { r32 p0; Vec2 yz; r32 p1; };
    struct { Vec3 xyz; r32 p2; };
    struct { r32 p3; Vec3 yzw; };
    
    struct { r32 r, g, b, a; };
    struct { Vec2 rg; Vec2 ba; };
    struct { r32 p4; Vec2 gb; r32 p5; };
    struct { Vec3 rgb; r32 p6; };
    struct { r32 p7; Vec3 gba; };
} Vec4;

typedef union
{
    r32 p[3][3];
    struct { Vec3 c0, c1, c2; };
} Mat3;

typedef union
{
    r32 p[4][4];
    struct { Vec4 c0, c1, c2, c3; };
} Mat4;

typedef union
{
    r32 p[4];
    struct { r32 x, y, z, w; };
    struct { Vec3 xyz; r32 theta; };
} Quaternion;

typedef Vec2       v2;
typedef Vec3       v3;
typedef Vec4       v4;
typedef Vec3       c3; // 3 component color (rgb)
typedef Vec4       c4; // 4 component color (rgba)
typedef Mat3       m3;
typedef Mat4       m4;
typedef Quaternion qt;

#define V2_ZERO { 0.0f, 0.0f }
#define V3_ZERO { 0.0f, 0.0f, 0.0f}
#define V4_ZERO { 0.0f, 0.0f, 0.0f, 0.0f }
#define V2_ONE  { 1.0f, 1.0f }
#define V3_ONE  { 1.0f, 1.0f, 1.0f }
#define V4_ONE  { 1.0f, 1.0f, 1.0f, 1.0f }

#define M3_IDENTITY m3_diag(1.0f) 
#define M4_IDENTITY m4_diag(1.0f) 

inline r32 degrees_to_radians(r32 theta)
{
    return theta * (MM_PI / 180.0f);
}

static r32 lerp(r32 v0, r32 v1, r32 t);
r32 inv_lerp(r32 v0, r32 v1, r32 v);
r32 remap(r32 i0, r32 i1, r32 o0, r32 o1, r32 v);

// Vec2 Pre-decs

v2  v2_add(v2 left, v2 right);
v2  v2_sub(v2 left, v2 right);
v2  v2_mul(v2 left, v2 right);
v2  v2_mulf(v2 left, r32 right);
v2  v2_divf(v2 left, r32 right);
r32 v2_dot(v2 left, v2 right);
/* Computes z-component in 3D space on an xy plane */
r32 v2_cross(v2 left, v2 right);
v2  v2_norm(v2 left);
r32 v2_mag(v2 left);
r32 v2_mag_sq(v2 left);

// Vec3 Pre-decs

v3  v3_init(r32 x, r32 y, r32 z);
v3  v3_init(const r32 p[3]);
v3  v3_add(v3 left, v3 right);
v3  v3_sub(v3 left, v3 right);
v3  v3_mul(v3 left, v3 right);
v3  v3_mulf(v3 left, r32 right);
v3  v3_divf(v3 left, r32 right);
r32 v3_dot(v3 left, v3 right);
v3  v3_cross(v3 left, v3 right);
v3  v3_norm(v3 left);
r32 v3_mag(v3 left);
r32 v3_mag_sq(v3 left);

v3 v3_compute_normal(v3 a, v3 b, v3 c);

// Vec4 Pre-decs

v4  v4_add(v4 left, v4 right);
v4  v4_sub(v4 left, v4 right);
v4  v4_mul(v4 left, v4 right);
v4  v4_mulf(v4 left, r32 right);
v4  v4_divf(v4 left, r32 right);
r32 v4_dot(v4 left, v4 right);
/* Takes the cross product of xyz components and sets w = 1.0f */
v4  v4_cross(v4 left, v4 right);
v4  v4_norm(v4 left);
r32 v4_mag(v4 left);
r32 v4_mag_sq(v4 left);

// MAT3 Pre-decs

m3 m3_diag(r32 d);

// MAT4 Pre-decs

m4 m4_diag(r32 d);
m4 m3_to_m4(m3 mat);
static m3 m4_to_m3(m4 mat);
m4 m4_mul(m4 left, m4 right);
/* Creates a scaling matrix */
m4 m4_scale(r32 sx, r32 sy, r32 sz);
/* Creates a translation matrix */
m4 m4_translate(v3 trans);
/* Creates a look at matrix */
static m4 m4_look_at(v3 eye, v3 center, v3 up);
/* Creates a perspective projection matrix */
m4 m4_perspective(r32 fov, r32 ar, r32 near, r32 far);
m4 m4_rotate_x(r32 theta);
m4 m4_rotate_y(r32 theta);
m4 m4_rotate_z(r32 theta);
m4 m4_rotate(r32 theta, v3 axis);

static v4 m4_mul_v4(m4 m, v4 v);
static m4 m4_inverse(m4 m);

// QUATERNION Pre-decs
qt qt_init(r32 x, r32 y, r32 z, r32 w);
qt qt_init(const r32 p[4]);
qt qt_norm(qt q);
qt euler_to_qt(const r32 roll, const r32 pitch, const r32 yaw);
qt euler_to_qt(const r32 euler[3]);
void qt_to_euler(const qt q, r32& roll, r32& pitch, r32& yaw);
void qt_to_euler(const qt quat, r32 euler[3]);
static v3 qt_transform(v3 v, qt q);

// Other Utility Pre-decs
r32 clamp(r32 min, r32 max, r32 val);
r32 smoothstep(r32 v0, r32 v1, r32 t);
r32 smootherstep(r32 v0, r32 v1, r32 t);

static r32 random();
static r32 random_clamped(r32 Min, r32 Max);
static i32 random_int_clamped(i32 Min, i32 Max);
static v3 v3_random();
static v3 v3_random_clamped(r32 Min, r32 Max);
static v3 random_in_unit_sphere();
static v3 random_in_hemisphere(v3 Normal);
static v3 random_unit_vector();
static v3 random_in_unit_disc();
static r32 schlick(r32 cosine, r32 ref_idx);
static v3 refract(v3 uv, v3 n, r32 ratio);
static v3 reflect(v3 v, v3 normal);

FORCE_INLINE bool 
v3_near_zero(v3 v) 
{
    // Return true if the vector is close to zero in all dimensions.
    const auto s = 1e-8;
    return (fabsf(v.p[0]) < s) && (fabsf(v.p[1]) < s) && (fabsf(v.p[2]) < s);
}

#pragma warning(default : 4201)

#endif //_MAPLE_MATH_H

#if defined(MAPLE_MATH_IMPLEMENTATION)

r32 
lerp(r32 v0, r32 v1, r32 t)
{
    return v0 + t * (v1 - v0);
}

r32 
inv_lerp(r32 v0, r32 v1, r32 v)
{
    r32 Result = 0.0f;
    
    Result = (v - v0) / (v1 - v0);
    Result = (Result < 0.0f) ? 0.0f : (Result > 1.0f) ? 1.0f : Result;
    
    return Result;
}

r32 
remap(r32 i0, r32 i1, r32 o0, r32 o1, r32 v)
{
    r32 t = inv_lerp(i0, i1, v);
    return lerp(o0, o1, t);
}

// Vec2 Pre-decs

v2 
v2_add(v2 left, v2 right)
{
    v2 result;
    
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    
    return result;
}

v2 
v2_sub(v2 left, v2 right)
{
    v2 result;
    
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    
    return result;
}

v2 
v2_mul(v2 left, v2 right)
{
    v2 result;
    
    result.x = left.x * right.x;
    result.y = left.y * right.y;
    
    return result;
}

v2
v2_mulf(v2 left, r32 right)
{
    v2 result;
    
    result.x = left.x * right;
    result.y = left.y * right;
    
    return result;
}

v2 
v2_divf(v2 left, r32 right)
{
    assert(right != 0.0f && "Attempted to divide a Vec2 by 0.0f");
    
    v2 result;
    
    result.x = left.x / right;
    result.y = left.y / right;
    
    return result;
}

r32 
v2_dot(v2 left, v2 right)
{
    r32 result = 0.0f;
    
    result += left.x * right.x;
    result += left.y * right.y;
    
    return result;
}

r32 
v2_cross(v2 left, v2 right)
{
    r32 result;
    result = (left.x * right.y) - (left.y * right.x);
    return result;
}

v2 
v2_norm(v2 left)
{
    v2 result;
    
    r32 mag = 1.0f / v2_mag(left);
    result.x = left.x * mag;
    result.y = left.y * mag;
    
    return result;
}

r32 
v2_mag(v2 left)
{
    r32 result;
    result = sqrtf(left.x * left.x + left.y * left.y);
    return result;
}

r32 
v2_mag_sq(v2 left)
{
    r32 result;
    result = left.x * left.x + left.y * left.y;
    return result;
}

// Vec3 Pre-decs

v3 
v3_init(r32 x, r32 y, r32 z) 
{
    v3 result{};
    
    result.x = x;
    result.x = y;
    result.x = z;
    
    return result;
}

v3 
v3_init(const r32 p[3])
{
    v3 result{};
    
    result.x = p[0];
    result.x = p[1];
    result.x = p[2];
    
    return result;
}

v3
v3_add(v3 left, v3 right)
{
    v3 result;
    
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;
    
    return result;
}

v3 
v3_sub(v3 left, v3 right)
{
    v3 result;
    
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    result.z = left.z - right.z;
    
    return result;
}

v3 
v3_mul(v3 left, v3 right)
{
    v3 result;
    
    result.x = left.x * right.x;
    result.y = left.y * right.y;
    result.z = left.z * right.z;
    
    return result;
}

v3 
v3_mulf(v3 left, r32 right)
{
    v3 result;
    
    result.x = left.x * right;
    result.y = left.y * right;
    result.z = left.z * right;
    
    return result;
}

v3 
v3_divf(v3 left, r32 right)
{
    assert(right != 0.0f && "Attempted to divide a Vec2 by 0.0f");
    
    v3 result;
    
    result.x = left.x / right;
    result.y = left.y / right;
    result.z = left.z / right;
    
    return result;
}

r32 
v3_dot(v3 left, v3 right)
{
    r32 result = 0.0f;
    
    result += left.x * right.x;
    result += left.y * right.y;
    result += left.z * right.z;
    
    return result;
}

v3 
v3_cross(v3 left, v3 right)
{
    v3 result;
    
    result.x = (left.y * right.z) - (left.z * right.y);
    result.y = (left.z * right.x) - (left.x * right.z);
    result.z = (left.x * right.y) - (left.y * right.x);
    
    return result;
}

v3 v3_norm(v3 left)
{
    v3 result;
    
    r32 mag = 1.0f / v3_mag(left);
    result.x = left.x * mag;
    result.y = left.y * mag;
    result.z = left.z * mag;
    
    return result;
}

r32 v3_mag(v3 left)
{
    r32 result;
    result = sqrtf(left.x * left.x + left.y * left.y + left.z * left.z);
    return result;
}

r32 v3_mag_sq(v3 left)
{
    r32 result;
    result = left.x * left.x + left.y * left.y + left.z * left.z;
    return result;
}

// Vec4 Pre-decs

v4 v4_add(v4 left, v4 right)
{
    v4 result;
    
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;
    result.w = left.w + right.w;
    
    return result;
}

v4 v4_sub(v4 left, v4 right)
{
    v4 result;
    
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    result.z = left.z - right.z;
    result.w = left.w - right.w;
    
    return result;
}

v4 v4_mul(v4 left, v4 right)
{
    v4 result;
    
    result.x = left.x * right.x;
    result.y = left.y * right.y;
    result.z = left.z * right.z;
    result.w = left.w * right.w;
    
    return result;
}

v4 v4_mulf(v4 left, r32 right)
{
    v4 result;
    
    result.x = left.x * right;
    result.y = left.y * right;
    result.z = left.z * right;
    result.w = left.w * right;
    
    return result;
}

v4 v4_divf(v4 left, r32 right)
{
    assert(right != 0.0f && "Attempted to divide a Vec4 by 0.0f");
    
    v4 result;
    
    result.x = left.x / right;
    result.y = left.y / right;
    result.z = left.z / right;
    result.w = left.w / right;
    
    return result;
}

r32 v4_dot(v4 left, v4 right)
{
    r32 result = 0.0f;
    
    result += left.x * right.x;
    result += left.y * right.y;
    result += left.z * right.z;
    result += left.w * right.w;
    
    return result;
}

v4 v4_cross(v4 left, v4 right)
{
    v4 result;
    
    result.xyz = v3_cross(left.xyz, right.xyz);
    result.w = 1.0f;
    
    return result;
}

v4 v4_norm(v4 left)
{
    v4 result;
    
    r32 mag = 1.0f / v4_mag(left);
    result.x = left.x * mag;
    result.y = left.y * mag;
    result.z = left.z * mag;
    result.w = left.w * mag;
    
    return result;
}

r32 v4_mag(v4 left)
{
    r32 result;
    result = sqrtf(left.x * left.x + left.y * left.y + left.z * left.z + left.w * left.w);
    return result;
}

r32 v4_mag_sq(v4 left)
{
    r32 result;
    result = left.x * left.x + left.y * left.y + left.z * left.z + left.w * left.w;
    return result;
}

// MAT3 Defs

m3 m3_diag(r32 d)
{
    v3 c0 = { d,    0.0f, 0.0f };
    v3 c1 = { 0.0f, d,    0.0f };
    v3 c2 = { 0.0f, 0.0f, d };
    
    m3 result;
    
    result.c0 = c0;
    result.c1 = c1;
    result.c2 = c2;
    
    return result;
}

// MAT4 Pre-decs

m4 m4_diag(r32 d)
{
    m4 result;
    
    v4 c0 = { d,    0.0f, 0.0f, 0.0f };
    v4 c1 = { 0.0f, d,    0.0f, 0.0f };
    v4 c2 = { 0.0f, 0.0f, d   , 0.0f };
    v4 c3 = { 0.0f, 0.0f, 0.0f, d };
    
    result.c0 = c0;
    result.c1 = c1;
    result.c2 = c2;
    result.c3 = c3;
    
    return result;
}

m4 m3_to_m4(m3 mat)
{
    m4 result = M4_IDENTITY;
    
    result.c0.xyz = mat.c0;
    result.c1.xyz = mat.c1;
    result.c2.xyz = mat.c2;
    
    return result;
}

static m3
m4_to_m3(m4 mat)
{
    m3 result = M3_IDENTITY;
    
    result.c0 = mat.c0.xyz;
    result.c1 = mat.c1.xyz;
    result.c2 = mat.c2.xyz;
    
    return result;
}

m4 m4_mul(m4 left, m4 r)
{
    m4 result;
    
    v4 lr0 = { left.p[0][0], left.p[1][0], left.p[2][0], left.p[3][0] };
    v4 lr1 = { left.p[0][1], left.p[1][1], left.p[2][1], left.p[3][1] };
    v4 lr2 = { left.p[0][2], left.p[1][2], left.p[2][2], left.p[3][2] };
    v4 lr3 = { left.p[0][3], left.p[1][3], left.p[2][3], left.p[3][3] };
    
    result.p[0][0] = v4_dot(lr0, r.c0);
    result.p[0][1] = v4_dot(lr1, r.c0);
    result.p[0][2] = v4_dot(lr2, r.c0);
    result.p[0][3] = v4_dot(lr3, r.c0);
    
    result.p[1][0] = v4_dot(lr0, r.c1);
    result.p[1][1] = v4_dot(lr1, r.c1);
    result.p[1][2] = v4_dot(lr2, r.c1);
    result.p[1][3] = v4_dot(lr3, r.c1);
    
    result.p[2][0] = v4_dot(lr0, r.c2);
    result.p[2][1] = v4_dot(lr1, r.c2);
    result.p[2][2] = v4_dot(lr2, r.c2);
    result.p[2][3] = v4_dot(lr3, r.c2);
    
    result.p[3][0] = v4_dot(lr0, r.c3);
    result.p[3][1] = v4_dot(lr1, r.c3);
    result.p[3][2] = v4_dot(lr2, r.c3);
    result.p[3][3] = v4_dot(lr3, r.c3);
    
    return result;
}

// https://gist.github.com/mattatz/86fff4b32d198d0928d0fa4ff32cf6fa
static m4 
m4_inverse(m4 m)
{
    float n11 = m.p[0][0], n12 = m.p[1][0], n13 = m.p[2][0], n14 = m.p[3][0];
    float n21 = m.p[0][1], n22 = m.p[1][1], n23 = m.p[2][1], n24 = m.p[3][1];
    float n31 = m.p[0][2], n32 = m.p[1][2], n33 = m.p[2][2], n34 = m.p[3][2];
    float n41 = m.p[0][3], n42 = m.p[1][3], n43 = m.p[2][3], n44 = m.p[3][3];
    
    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;
    
    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;
    
    m4 ret = M4_IDENTITY;
    
    ret.p[0][0] = t11 * idet;
    ret.p[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret.p[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret.p[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;
    
    ret.p[1][0] = t12 * idet;
    ret.p[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret.p[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret.p[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;
    
    ret.p[2][0] = t13 * idet;
    ret.p[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret.p[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret.p[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;
    
    ret.p[3][0] = t14 * idet;
    ret.p[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret.p[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret.p[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;
    
    return ret;
}

/* Creates a scaling matrix */
m4 m4_scale(r32 sx, r32 sy, r32 sz)
{
    m4 result = M4_IDENTITY;
    
    result.p[0][0] = sx;
    result.p[1][1] = sy;
    result.p[2][2] = sz;
    
    return result;
}

static m4
m4_transpose(m4 in)
{
    m4 result = {};
    
    result.p[0][0] = in.p[0][0];
    result.p[0][1] = in.p[1][0];
    result.p[0][2] = in.p[2][0];
    result.p[0][3] = in.p[3][0];
    
    result.p[1][0] = in.p[0][1];
    result.p[1][1] = in.p[1][1];
    result.p[1][2] = in.p[2][1];
    result.p[1][3] = in.p[3][1];
    
    result.p[2][0] = in.p[0][2];
    result.p[2][1] = in.p[1][2];
    result.p[2][2] = in.p[2][2];
    result.p[2][3] = in.p[3][2];
    
    result.p[3][0] = in.p[0][3];
    result.p[3][1] = in.p[1][3];
    result.p[3][2] = in.p[2][3];
    result.p[3][3] = in.p[3][3];
    
    return result;
}

static v4  
m4_mul_v4(m4 m, v4 v)
{
    v4 r0 = { m.p[0][0], m.p[1][0], m.p[2][0], m.p[3][0] };
    v4 r1 = { m.p[0][1], m.p[1][1], m.p[2][1], m.p[3][1] };
    v4 r2 = { m.p[0][2], m.p[1][2], m.p[2][2], m.p[3][2] };
    v4 r3 = { m.p[0][3], m.p[1][3], m.p[2][3], m.p[3][3] };
    
    v4 result = {};
    result.x = v4_dot(v, r0);
    result.y = v4_dot(v, r1);
    result.z = v4_dot(v, r2);
    result.w = v4_dot(v, r3);
    return result;
}

/* Creates a translation matrix */
m4 m4_translate(v3 trans)
{
    m4 result = M4_IDENTITY;
    
    result.p[3][0] = trans.x;
    result.p[3][1] = trans.y;
    result.p[3][2] = trans.z;
    
    return result;
}

/* Creates a look at matrix */
static m4 
m4_look_at(v3 eye, v3 center, v3 up)
{
    m4 result = M4_IDENTITY;
    
#if 1
    
    v3 f = v3_norm(v3_sub(center, eye));
    v3 s = v3_norm(v3_cross(f, up));
    v3 u = v3_cross(s, f);
    
    result.p[0][0] = s.x;
    result.p[0][1] = u.x;
    result.p[0][2] = -f.x;
    result.p[0][3] = 0.0f;
    
    result.p[1][0] = s.y;
    result.p[1][1] = u.y;
    result.p[1][2] = -f.y;
    result.p[1][3] = 0.0f;
    
    result.p[2][0] = s.z;
    result.p[2][1] = u.z;
    result.p[2][2] = -f.z;
    result.p[2][3] = 0.0f;
    
    result.p[3][0] = -v3_dot(s, eye);
    result.p[3][1] = -v3_dot(u, eye);
    result.p[3][2] = v3_dot(f, eye);
    result.p[3][3] = 1.0f;
    
#else
    
    // Left Handed View Matrix
    
    v3 r2 = v3_norm(v3_sub(center, eye));
    v3 r0 = v3_norm(v3_cross(up, r2));
    v3 r1 = v3_cross(r2, r0);
    
    v3 neg_eye = v3_mulf(eye, -1.0f);
    
    r32 d0 = v3_dot(r0, neg_eye);
    r32 d1 = v3_dot(r1, neg_eye);
    r32 d2 = v3_dot(r2, neg_eye);
    
    result.p[0][0] = r0.x;
    result.p[1][0] = r0.y;
    result.p[2][0] = r0.z;
    result.p[3][0] = d0;
    
    result.p[0][1] = r1.x;
    result.p[1][1] = r1.y;
    result.p[2][1] = r1.z;
    result.p[3][1] = d1;
    
    result.p[0][2] = r2.x;
    result.p[1][2] = r2.y;
    result.p[2][2] = r2.z;
    result.p[3][2] = d2;
    
    result.p[0][3] = 0.0f;
    result.p[1][3] = 0.0f;
    result.p[2][3] = 0.0f;
    result.p[3][3] = 1.0f;
    
#endif
    
    return result;
}

/* Creates a perspective projection matrix */
m4 m4_perspective(r32 fov, r32 ar, r32 near_plane, r32 far_plane)
{
    m4 result = M4_IDENTITY;
    
#if 1
    r32 cotangent = 1.0f / tanf(fov * ((r32)MM_PI / 360.0f));
    
    result.p[0][0] = cotangent / ar;
    result.p[1][1] = cotangent;
    result.p[2][3] = -1.0f;
    result.p[2][2] = (near_plane + far_plane) / (near_plane - far_plane);
    result.p[3][2] = (2.0f * near_plane * far_plane) / (near_plane - far_plane);
    result.p[3][3] = 0.0f;
#else
    
    // Left Handed Projection Matrix
    
    r32 radians = fov * ((r32)MM_PI / 360.0f);
    r32 cos_fov = cosf(radians);
    r32 sin_fov = sinf(radians);
    
    r32 height = cos_fov / sin_fov; // cot
    r32 width  = height / ar;
    r32 frange = far_plane / (far_plane - near_plane);
    
    result.p[0][0] = width;
    result.p[1][1] = height;
    result.p[2][2] = frange;
    result.p[2][3] = 1.0f;
    result.p[3][2] = -frange * near_plane;
    result.p[3][3] = 0.0f;
#endif
    
    return result;
}

m4 m4_rotate_x(r32 theta)
{
    r32 rad = degrees_to_radians(theta);
    
    r32 c = cosf(rad);
    r32 s = sinf(rad);
    
    m4 result;
    
    result.p[0][0] = 1.0f;
    result.p[0][1] = 0.0f;
    result.p[0][2] = 0.0f;
    result.p[0][3] = 0.0f;
    
    result.p[1][0] = 0.0f;
    result.p[1][1] = c;
    result.p[1][2] = s;
    result.p[1][3] = 0.0f;
    
    result.p[2][0] = 0.0f;
    result.p[2][1] = -s;
    result.p[2][2] = c;
    result.p[2][3] = 0.0f;
    
    result.p[3][0] = 0.0f;
    result.p[3][1] = 0.0f;
    result.p[3][2] = 0.0f;
    result.p[3][3] = 1.0f;
    
    return result;
}

m4 m4_rotate_y(r32 theta)
{
    r32 rad = degrees_to_radians(theta);
    
    r32 c = cosf(rad);
    r32 s = sinf(rad);
    
    m4 result;
    
    result.p[0][0] = c;
    result.p[0][1] = 0.0f;
    result.p[0][2] = -s;
    result.p[0][3] = 0.0f;
    
    result.p[1][0] = 0.0f;
    result.p[1][1] = 1.0f;
    result.p[1][2] = 0.0f;
    result.p[1][3] = 0.0f;
    
    result.p[2][0] = s;
    result.p[2][1] = 0.0f;
    result.p[2][2] = c;
    result.p[2][3] = 0.0f;
    
    result.p[3][0] = 0.0f;
    result.p[3][1] = 0.0f;
    result.p[3][2] = 0.0f;
    result.p[3][3] = 1.0f;
    
    return result;
}

m4 m4_rotate_z(r32 theta)
{
    r32 rad = degrees_to_radians(theta);
    
    r32 c = cosf(rad);
    r32 s = sinf(rad);
    
    m4 result;
    
    result.p[0][0] = c;
    result.p[0][1] = s;
    result.p[0][2] = 0.0f;
    result.p[0][3] = 0.0f;
    
    result.p[1][0] = -s;
    result.p[1][1] = c;
    result.p[1][2] = 0.0f;
    result.p[1][3] = 0.0f;
    
    result.p[2][0] = 0.0f;
    result.p[2][1] = 0.0f;
    result.p[2][2] = 1.0f;
    result.p[2][3] = 0.0f;
    
    result.p[3][0] = 0.0f;
    result.p[3][1] = 0.0f;
    result.p[3][2] = 0.0f;
    result.p[3][3] = 1.0f;
    
    return result;
}

m4 m4_rotate(r32 theta, v3 axis)
{
    r32 rad = degrees_to_radians(theta);
    axis = v3_norm(axis);
    
    r32 c = cosf(rad);
    r32 s = sinf(rad);
    r32 d = 1.0f - c;
    
    r32 x = axis.x * d;
    r32 y = axis.y * d;
    r32 z = axis.z * d;
    r32 axay = x * axis.y;
    r32 axaz = x * axis.z;
    r32 ayaz = y * axis.z;
    
    m4 result;
    
    result.p[0][0] = c + x * axis.x;
    result.p[0][1] = axay + s * axis.z;
    result.p[0][2] = axaz - s * axis.y;
    result.p[0][3] = 0.0f;
    
    result.p[1][0] = axay - s * axis.z;
    result.p[1][1] = c + y * axis.y;
    result.p[1][2] = ayaz + s * axis.x;
    result.p[1][3] = 0.0f;
    
    result.p[2][0] = axaz + s * axis.y;
    result.p[2][1] = ayaz - s * axis.x;
    result.p[2][2] = c + z * axis.z;
    result.p[2][3] = 0.0f;
    
    result.p[3][0] = 0.0f;
    result.p[3][1] = 0.0f;
    result.p[3][2] = 0.0f;
    result.p[3][3] = 1.0f;
    
    return result;
}

qt
qt_init(r32 x, r32 y, r32 z, r32 w)
{
    qt result{};
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}

qt
qt_init(const r32 p[4])
{
    qt result{};
    
    result.x = p[0];
    result.y = p[1];
    result.z = p[2];
    result.w = p[3];
    
    return result;
}

qt
qt_norm(qt q)
{
    r32 d = 1.0f / sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    
    qt result{};
    
    result.w = q.w * d;
    result.x = q.x * d;
    result.y = q.y * d;
    result.z = q.z * d;
    
    return result;
}

qt
euler_to_qt(const r32 r, const r32 p, const r32 y)
{
    const r32 roll  = degrees_to_radians(r);
    const r32 pitch = degrees_to_radians(p);
    const r32 yaw   = degrees_to_radians(y);
    
    r32 cy = cosf(yaw * 0.5f);
    r32 sy = sinf(yaw * 0.5f);
    r32 cp = cosf(pitch * 0.5f);
    r32 sp = sinf(pitch * 0.5f);
    r32 cr = cosf(roll * 0.5f);
    r32 sr = sinf(roll * 0.5f);
    
    qt result{};
    
    result.w = cr * cp * cy + sr * sp * sy;
    result.x = sr * cp * cy - cr * sp * sy;
    result.y = cr * sp * cy + sr * cp * sy;
    result.z = cr * cp * sy - sr * sp * cy;
    
    return qt_norm(result);
}

qt
euler_to_qt(const r32 euler[3])
{
    const r32 roll  = degrees_to_radians(euler[0]);
    const r32 pitch = degrees_to_radians(euler[1]);
    const r32 yaw   = degrees_to_radians(euler[2]);
    
    r32 cy = cosf(yaw * 0.5f);
    r32 sy = sinf(yaw * 0.5f);
    r32 cp = cosf(pitch * 0.5f);
    r32 sp = sinf(pitch * 0.5f);
    r32 cr = cosf(roll * 0.5f);
    r32 sr = sinf(roll * 0.5f);
    
    qt result{};
    
    result.w = cr * cp * cy + sr * sp * sy;
    result.x = sr * cp * cy - cr * sp * sy;
    result.y = cr * sp * cy + sr * cp * sy;
    result.z = cr * cp * sy - sr * sp * cy;
    
    return qt_norm(result);
}

void
qt_to_euler(const qt q, r32& roll, r32& pitch, r32& yaw)
{
    // roll (x-axis rotation)
    r32 sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    r32 cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    roll = atan2f(sinr_cosp, cosr_cosp);
    
    // pitch (y-axis rotation)
    r32 sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (fabsf(sinp) >= 1.0f)
    {
        pitch = copysignf(MM_PI / 2.0f, sinp);
    }
    else
    {
        pitch = asinf(sinp);
    }
    
    // yaw (z-axis rotation)
    r32 siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    r32 cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    yaw = atan2f(siny_cosp, cosy_cosp);
}

void
qt_to_euler(const qt q, r32 euler[3])
{
    // roll (x-axis rotation)
    r32 sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    r32 cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    euler[0] = atan2f(sinr_cosp, cosr_cosp);
    
    // pitch (y-axis rotation)
    r32 sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (fabsf(sinp) >= 1.0f)
    {
        euler[1] = copysignf(MM_PI / 2.0f, sinp);
    }
    else
    {
        euler[1] = asinf(sinp);
    }
    
    // yaw (z-axis rotation)
    r32 siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    r32 cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    euler[2] = atan2f(siny_cosp, cosy_cosp);
}

static v3 
qt_transform(v3 v, qt q)
{
    v3 result = {};
    
    r32 b2 = q.p[0]*q.p[0] + q.p[1]*q.p[1] + q.p[2]*q.p[2];
    
    v3 p1 = v3_mulf(v, (q.w * q.w - b2));
    v3 p2 = v3_mulf(q.xyz, v3_dot(q.xyz, v) * 2.0f);
    v3 p3 = v3_mulf(v3_cross(q.xyz, v), (q.w * 2.0f));
    
    return v3_add(v3_add(p1, p2), p3);
}


r32 clamp(r32 min, r32 max, r32 val)
{
    return (val < min) ? min : (val > max) ? max : val;
}

r32 smoothstep(r32 v0, r32 v1, r32 t)
{
    t = clamp(0.0f, 1.0f, (t - v0) / (v1 - v0));
    return t * t * (3 - 2 * t);
}

r32 smootherstep(r32 v0, r32 v1, r32 t)
{
    t = clamp(0.0f, 1.0f, (t - v0) / (v1 - v0));
    return t * t * t * (t * (t * 6 - 15) + 10);
}

v3 v3_compute_normal(v3 a, v3 b, v3 c)
{
    v3 ba = v3_sub(b, a);
    v3 ca = v3_sub(c, a);
    return v3_norm(v3_cross(ba, ca));
}

static r32 random()
{
    return rand() / (RAND_MAX + 1.0f);
}

static r32 random_clamped(r32 Min, r32 Max)
{
    return Min + (Max - Min) * random();
}

static i32 random_int_clamped(i32 Min, i32 Max)
{
    i32 Rand = rand();
    return Min + Rand % (Max - Min);
}

static v3 v3_random()
{
    v3 Result;
    
    Result.x = random();
    Result.y = random();
    Result.z = random();
    
    return Result;
}

static v3 v3_random_clamped(r32 Min, r32 Max)
{
    v3 Result;
    
    Result.x = random_clamped(Min, Max);
    Result.y = random_clamped(Min, Max);
    Result.z = random_clamped(Min, Max);
    
    return Result;
}

static v3 random_in_unit_sphere()
{
    while (true)
    {
        v3 Ran = v3_random_clamped(-1.0f, 1.0f);
        if (v3_mag_sq(Ran) >= 1.0f) continue;
        return Ran;
    }
}

static v3 random_in_hemisphere(v3 Normal)
{
    v3 RandomInSphere = random_in_unit_sphere();
    if (v3_dot(RandomInSphere, Normal) > 0.0f)
    {
        return RandomInSphere;
    }
    else
    {
        return v3_mulf(RandomInSphere, -1.0f);
    }
}

static v3 random_unit_vector()
{
    r32 A = random_clamped(0, 2 * MM_PI);
    r32 Z = random_clamped(-1, 1);
    r32 R = sqrtf(1 - Z * Z);
    return { R * cosf(A), R * sinf(A), Z };
}

static v3 random_in_unit_disc()
{
    while (true)
    {
        v3 Ran = { random_clamped(-1, 1), random_clamped(-1, 1), 0};
        if (v3_mag_sq(Ran) >= 1.0f) continue;
        return Ran;
    }
}

static v3 reflect(v3 v, v3 normal)
{
    return v3_sub(v, v3_mulf(v3_mulf(normal, v3_dot(v, normal)), 2.0f));
}

static v3 refract(v3 uv, v3 n, r32 ratio)
{
    r32 CosTheta = v3_dot(v3_mulf(uv, -1.0f), n);
    v3 RoutParallel = v3_mulf(v3_add(uv, v3_mulf(n, CosTheta)), ratio);
    v3 RoutPerp = v3_mulf(n, -sqrtf(1.0f - v3_mag_sq(RoutParallel)));
    return v3_add(RoutParallel, RoutPerp);
}

static r32 schlick(r32 cosine, r32 ref_idx)
{
    r32 R0 = (1 - ref_idx) / (1 + ref_idx);
    R0 = R0 * R0;
    return R0 + (1 - R0) * powf((1 - cosine), 5);
}

#endif //MAPLE_MATH_IMPLEMENTATION