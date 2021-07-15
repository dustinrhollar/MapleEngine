
namespace editor
{
    // @FIXME: Temporary location. Move out so that the camera can be better generalized
    // @FIXME: 
    struct ViewportCamera
    {
        // World data
        v3 _position;
        v3 _front;
        v3 _up;
        v3 _right;
        v3 _world_up;
        // Euler Rotation
        r32 _yaw;
        r32 _pitch;
        // Other data
        r32 _speed;
        r32 _mouse_sens;
        r32 _zoom;
        v2  _mouse_pos;
        // If bit == 1, then button is pressed
        u32 _mouse_press_mask;
        // Windows will pausse for ~500miliseconds before repeating
        // a key down event. To avoid this pause when trying to move
        // the camera, cache a bitmask for key presses
        // Enum that represents the bit for each key press
        enum KeyPressMask
        {
            KeyMask_W = 0,
            KeyMask_S = 1,
            KeyMask_A = 2,
            KeyMask_D = 3,
            KeyMask_Q = 4,
            KeyMask_E = 5,
        };
        // If bit == 1, then button is pressed
        u32 _key_press_mask;
        
        void Init(v3  position = { 0.0f, 0.0f, 0.0f }, 
                  v3  up       = { 0.0f, 1.0f, 0.0f }, 
                  r32 yaw      = 90.0f, 
                  r32 pitch    = 0.0f)
        {
            _position   = position;
            _world_up   = up;
            _yaw        = yaw;
            _pitch      = pitch;
            _front      = { 0.0f, 0.0f, -1.0f };
            _speed      = 10.5f;
            _mouse_sens = 0.1f;
            _zoom       = 45.0f;
            _mouse_press_mask = 0;
            
            _mouse_pos.x = R32_MAX;
            _mouse_pos.y = R32_MAX;
            
            UpdateCameraVectors();
        }
        
        m4 LookAt()
        {
            return m4_look_at(_position, v3_add(_position, _front), _up);
        }
        
        void OnMouseButtonPress(int button)
        {
            BIT32_TOGGLE_1(_mouse_press_mask, button);
        }
        
        void OnMouseButtonRelease(int button)
        {
            BIT32_TOGGLE_0(_mouse_press_mask, button);
        }
        
        void OnUpdate(r32 delta_time, v2 delta_mouse)
        {
            //if (delta_mouse.x == 0 && delta_mouse.y == 0) return;
            
            // 3 modes of movement:
            // ---- LMB + RMB + Drag: Moves up and down.
            // ---- LMB + Drag:       Moves the camera forward and backward and rotates left and right.
            // ---- RMB + Drag:       Rotates the viewport camera.
            
            bool lmb_press = (_mouse_press_mask & BIT(0));
            bool rmb_press = (_mouse_press_mask & BIT(1));
            
            r32 xoffset = delta_mouse.x * _mouse_sens;
            r32 yoffset = delta_mouse.y * _mouse_sens;
            
            r32 velocity = _speed * delta_time;
            r32 velocity_x = _speed * xoffset;
            r32 velocity_y = _speed * yoffset;
            
            if (lmb_press && rmb_press)
            {
                _position = v3_sub(_position, v3_mulf(_up, velocity_y));
                _position = v3_add(_position, v3_mulf(_right, velocity_x));
            }
            else if (lmb_press)
            {
                // +y moves forward
                // -y moves backward
                // +x turn right
                // -x turn left
                _yaw -= xoffset;
                UpdateCameraVectors();
                
                _position = v3_sub(_position, v3_mulf(_front, yoffset));
            }
            else if (rmb_press)
            {
                _yaw   -= xoffset;
                _pitch += yoffset; // @FIXME: Figure out delta y doesn't work w/out a boost!
                // make sure that when pitch is out of bounds, screen doesn't get flipped
                //if (false /*constrain_pitch*/)
                {
                    if (_pitch > 89.0f)
                        _pitch = 89.0f;
                    if (_pitch < -89.0f)
                        _pitch = -89.0f;
                }
                
                UpdateCameraVectors();
                
                if ((_key_press_mask & BIT(KeyMask_W)))
                    _position = v3_add(_position, v3_mulf(_front, velocity));
                if ((_key_press_mask & BIT(KeyMask_S)))
                    _position = v3_sub(_position, v3_mulf(_front, velocity));
                if ((_key_press_mask & BIT(KeyMask_A)))
                    _position = v3_add(_position, v3_mulf(_right, velocity));
                if ((_key_press_mask & BIT(KeyMask_D)))
                    _position = v3_sub(_position, v3_mulf(_right, velocity));
                if ((_key_press_mask & BIT(KeyMask_Q)))
                    _position = v3_sub(_position, v3_mulf(_up, velocity));
                if ((_key_press_mask & BIT(KeyMask_E)))
                    _position = v3_add(_position, v3_mulf(_up, velocity));
            }
        }
        
        void OnKeyPress(MapleKey key)
        {
            if (key == Key_W)
                BIT32_TOGGLE_1(_key_press_mask, KeyMask_W);
            else if (key == Key_S)
                BIT32_TOGGLE_1(_key_press_mask, KeyMask_S);
            else if (key == Key_A)
                BIT32_TOGGLE_1(_key_press_mask, KeyMask_A);
            else if (key == Key_D)
                BIT32_TOGGLE_1(_key_press_mask, KeyMask_D);
            else if (key == Key_Q)
                BIT32_TOGGLE_1(_key_press_mask, KeyMask_Q);
            else if (key == Key_E)
                BIT32_TOGGLE_1(_key_press_mask, KeyMask_E);
        }
        
        void OnKeyRelease(MapleKey key)
        {
            if (key == Key_W)
                BIT32_TOGGLE_0(_key_press_mask, KeyMask_W);
            else if (key == Key_S)
                BIT32_TOGGLE_0(_key_press_mask, KeyMask_S);
            else if (key == Key_A)
                BIT32_TOGGLE_0(_key_press_mask, KeyMask_A);
            else if (key == Key_D)
                BIT32_TOGGLE_0(_key_press_mask, KeyMask_D);
            else if (key == Key_Q)
                BIT32_TOGGLE_0(_key_press_mask, KeyMask_Q);
            else if (key == Key_E)
                BIT32_TOGGLE_0(_key_press_mask, KeyMask_E);
        }
        
        // TODO(Dustin): Actually make use of this!
        void OnMouseScroll(float yoffset)
        {
            _zoom -= (float)yoffset;
            if (_zoom < 1.0f)
                _zoom = 1.0f;
            if (_zoom > 45.0f)
                _zoom = 45.0f; 
        }
        
        // calculates the front vector from the Camera's (updated) Euler Angles
        void UpdateCameraVectors()
        {
            r32 pitch_rad = degrees_to_radians(_pitch);
            r32 yaw_rad = degrees_to_radians(_yaw);
            
            // calculate the new Front vector
            v3 front;
            front.x = cosf(yaw_rad) * cosf(pitch_rad);
            front.y = sinf(pitch_rad);
            front.z = sinf(yaw_rad) * cosf(pitch_rad);
            
            _front = v3_norm(front);
            _right = v3_norm(v3_cross(_front, _world_up));  
            _up    = v3_norm(v3_cross(_right, _front));
        }
    };
}