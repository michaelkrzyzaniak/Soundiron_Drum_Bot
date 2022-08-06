/*---------------------_---------------_--------------------_----_------    _
   ____ __  ___ ___ __| |___ _ __  ___| |_ ___ _ _  __ __ _(_)__| |__ _ ___| |_ 
  (_-< '_ \/ -_) -_) _` / _ \ '  \/ -_)  _/ -_) '_| \ V  V / / _` / _` / -_)  _|
  /__/ .__/\___\___\__,_\___/_|_|_\___|\__\___|_|    \_/\_/|_\__,_\__, \___|\__|
-----|_|----------------------------------------------------------|___/-  
----------------------------------------------------------------------*/
//hex+alpha not working on android browser
/*
Speedometer_Widget.MAIN_COLOR      = "rgb(2, 95, 188)";
Speedometer_Widget.TOUCH_COLOR     = "rgb(255, 124, 0)";
Speedometer_Widget.MAIN_COLOR_A    = "rgba(2, 95, 188, 0.7)";
Speedometer_Widget.TOUCH_COLOR_A   = "rgba(255, 124, 0, 0.7)";
Speedometer_Widget.PASS_COLOR      = "#025FBC";
Speedometer_Widget.FONT            = "Quicksand";
*/
Speedometer_Widget.MAIN_COLOR      = "rgb(0, 0, 0)";
Speedometer_Widget.TOUCH_COLOR     = "rgb(255, 124, 0)";
Speedometer_Widget.MAIN_COLOR_A    = "rgba(32, 32, 32, 0.7)";
Speedometer_Widget.TOUCH_COLOR_A   = "rgba(255, 124, 0, 0.7)";
Speedometer_Widget.PASS_COLOR      = "black";
Speedometer_Widget.FONT            = "Verdana";

Speedometer_Widget.STYLE_TIMER       = 0;
Speedometer_Widget.STYLE_SPEEDOMETER = 1;

Speedometer_Widget.GOAL_TYPE_LESS_THAN    = 0;
Speedometer_Widget.GOAL_TYPE_AT_LEAST     = 1;
Speedometer_Widget.GOAL_TYPE_RANGE        = 2;
Speedometer_Widget.GOAL_TYPE_CONST_RANGE  = 3;
Speedometer_Widget.GOAL_TYPE_NONE         = 4;


//PUBLIC
/*--------------------------------------------------------------------*/
function Speedometer_Widget(canvas_id, level_text, num_text, style, goal_type)
{
  this.canvas             = document.getElementById(canvas_id);
  this.context            = this.canvas.getContext('2d');
  this.level_text         = level_text;
  this.num_text           = num_text;
  this.style              = style;
  this.goal_type          = goal_type;
  this.init();

  this.canvas.addEventListener("touchstart", this.touchstart.bind(this), false);
  this.canvas.addEventListener("touchend"  , this.touchend.bind(this)  , false);
  this.canvas.addEventListener("touchmove" , this.touchmove.bind(this)  , false);

  document.body.addEventListener("touchmove", this.prevent_touch_scroll.bind(this), {passive:false, capture:false});

  this.canvas.onmousedown = this.onmousedown.bind(this);
  //this.canvas.onmouseenter= this.onmousedown.bind(this);
  this.canvas.onmouseup   = this.onmouseup.bind  (this);
  this.canvas.onmouseout  = this.onmouseup.bind  (this);
  this.canvas.onmousemove = this.onmousemove.bind(this);
  
  this.is_updating   = false;
  this.update_thread = null;
  
  //for const range
  this.range         = 20;
}

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.init = function()
{
  this.outline_angle = 2.15;
  this.level_min     = 0; 
  this.level_max     = 200;
  this.num_min       = 0; 
  this.num_max       = 999;
  this.num_digits    = 0; //after decimal point
  this.level_digits  = 0; //after decimal point
  this.target_level  = 0;
  this.current_level = this.target_level;
  this.target_num    = 0;
  this.current_num   = this.target_num;
  this.target_goal   = this.level_max * 0.3;
  this.current_goal  = this.target_goal;
  this.goal_angle    = 0; //calculated when drawn for hit detection
  this.target_goal_2   = this.level_max;
  this.current_goal_2  = this.target_goal_2;
  this.goal_angle_2    = 0; //calculated when drawn for hit detection
  this.mouse_is_down = false;
  this.mouse_is_down_in_goal_indicator = false;
  this.mouse_is_down_in_goal_indicator_2 = false;
  this.interpolation_thread_runloop_duration = 75; //ms
  this.num_ticks     = 11;
  this.tick_spacing  = 2 * Math.PI / 15; //radians
  this.tick_start    = Math.floor(this.num_ticks * 0.5) * -this.tick_spacing;
  this.tick_end      = this.tick_start + this.tick_spacing * (this.num_ticks-1);
  
  if(this.style == Speedometer_Widget.STYLE_TIMER)
    {
      this.tick_start    = 0;
      this.num_ticks     = 9;
      this.tick_end      = 2 * Math.PI - Math.PI/(0.5*this.num_ticks);
      this.tick_spacing  = this.tick_end / (this.num_ticks-1);
      this.outline_angle = Math.PI;
      this.set_level_and_num_ranges(0, 16);
      this.target_goal   = 5;
      this.current_goal  = this.target_goal;
    }
  
  this.context.shadowColor = "#444";
  
  this.clear();
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.stop_updating = function()
{
  clearTimeout(this.update_thread);
  this.is_updating = false;
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.resume_updating = function()
{
  if(this.is_updating === false)
    this.interpolation_thread_run_loop_run();
  this.is_updating = true;
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.clear = function()
{
  this.set_level(this.level_min, true, this.num_min, true);
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.set_level_range = function(min, max)
{
  this.level_min = min; this.level_max = max;
  this.set_level(this.target_level, false, this.target_num, false);
  this.set_goal_range(this.target_goal, this.target_goal_2, false);
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.set_num_range = function(min, max)
{
  this.num_min = min; this.num_max = max;
  this.set_level(this.target_level, false, this.target_num, false);
  this.set_goal_range(this.target_goal, this.target_goal_2, false);
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.set_level_and_num_ranges = function(min, max)
{
  this.set_level_range(min, max);
  this.set_num_range(min, max);
};

//PUBLIC
//for const_range type
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.set_const_range = function(val)
{
  this.range = val;
};

//PUBLIC
//for range type
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.get_goal_mean = function(val)
{
  return 0.5 * (this.target_goal + this.target_goal_2);
};

//PUBLIC
/*--------------------------------------------------------------------*/
// the dial value is set independently of the center number.
// either can be set immediately, or interpolated
Speedometer_Widget.prototype.set_level = function(level, level_immediate, num, num_immediate)
{
  //interpolation_thread_run_loop_lock()
  this.target_level = this.clip(level, this.level_min, this.level_max);
  this.target_num   = this.clip(num  , this.num_min  , this.num_max  );
  
  if(level_immediate)
    this.current_level = this.target_level;
  if(num_immediate)
    this.current_num = this.target_num;    
  //interpolation_thread_run_loop_unlock()
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.set_goal = function(goal, immediate)
{
  //interpolation_thread_run_loop_lock()
  this.target_goal = this.clip(goal, this.level_min, this.level_max);  
  if(immediate)
    this.current_goal = this.target_goal;
  //interpolation_thread_run_loop_unlock()
};

/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.get_goal = function()
{
  return this.target_goal;
}

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.set_goal_2 = function(goal, immediate)
{
  //interpolation_thread_run_loop_lock()
  this.target_goal_2 = this.clip(goal, this.level_min, this.level_max);  
  if(immediate)
    this.current_goal_2 = this.target_goal_2;
  //interpolation_thread_run_loop_unlock()
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.set_goal_range = function(goal, goal_2, immediate)
{
  this.set_goal  (goal  , immediate);
  this.set_goal_2(goal_2, immediate);
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.set_challenge = function(less_than, at_least)
{
  var type = Speedometer_Widget.GOAL_TYPE_NONE;
  if((at_least !== "") && (less_than !== ""))
    {
      type  = Speedometer_Widget.GOAL_TYPE_RANGE;
      this.set_goal_range(at_least, less_than, false);
    }
  else if(less_than !== "")
    {
      type  = Speedometer_Widget.GOAL_TYPE_LESS_THAN;
      this.set_goal_range(less_than, less_than, false);
    }
  else if(at_least !== "")
   {
      type  = Speedometer_Widget.GOAL_TYPE_AT_LEAST;
      this.set_goal_range(at_least, at_least, false);  
   }
   
  this.goal_type = type;
  console.log(type);
};

//PRIVATE
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.draw = function(level, num, goal, goal_2)
{
  var i;
  var font_size;
  var red_line     = this.scalef(level, this.level_min, this.level_max, this.tick_start, this.tick_end);
  var zero_tick    = this.scalef(0    , this.level_min, this.level_max, this.tick_start, this.tick_end);
  this.goal_angle  = this.scalef(goal , this.level_min, this.level_max, this.tick_start, this.tick_end);
  this.goal_angle_2= this.scalef(goal_2,this.level_min, this.level_max, this.tick_start, this.tick_end);
  var big_radius   = 0.5 * Math.min(this.canvas.width, this.canvas.height); //to the edge of the drawing
  var radius       = 0.85 * big_radius; //to the edge of the clock
  this.context.clearRect(0, 0, this.canvas.width, this.canvas.height);
  
  //this.context.drawImage(this.background, 0.5*this.canvas.width - radius, 0.5*this.canvas.height - radius, 2*radius, 2*radius); 
  this.context.beginPath();
  this.context.lineCap="round";
  this.context.strokeStyle = Speedometer_Widget.MAIN_COLOR;
  this.context.lineWidth   = 0.025 * radius;
  this.context.arc         (big_radius, big_radius, radius, 1.5*Math.PI - this.outline_angle, 1.5*Math.PI + this.outline_angle);
  this.context.stroke();

  this.context.textBaseline = "middle";
  this.context.textAlign    = "center"; 
  
  this.context.save();
  this.context.translate(0.5*this.canvas.width, 0.5*this.canvas.height);
  
  //draw ticks and numbers
  this.context.save();
  this.context.rotate(this.tick_start);
  this.context.font = 0.12*radius + "px " + Speedometer_Widget.FONT;
  this.context.fillStyle = Speedometer_Widget.MAIN_COLOR;
  
  for(i=0; i<this.num_ticks; i++)
    {
      var n = this.scalef(i, 0, this.num_ticks-1, this.level_min, this.level_max);
      this.context.beginPath   ();
      this.context.moveTo      (0, 0.875 * -radius);
      this.context.lineTo      (0, 0.925 * -radius);
      this.context.lineWidth   = 4;
      this.context.strokeStyle = Speedometer_Widget.MAIN_COLOR;
      this.context.stroke      ();
      this.context.fillText    (Math.round(n), 0, -radius*0.75);
      this.context.rotate      (this.tick_spacing);
    }
  this.context.restore();
  
  this.context.lineWidth   = 0.1 * radius;
  //draw red or green arc to red line
  if(this.value_is_in_goal_range() === 0)
    this.context.strokeStyle   = Speedometer_Widget.TOUCH_COLOR_A;// + "B2";
  else
    this.context.strokeStyle   = Speedometer_Widget.MAIN_COLOR_A;// + "B2";
  
  this.context.beginPath   ();
  if(this.style == Speedometer_Widget.STYLE_TIMER)
    this.context.arc(0, 0, 0.9 * radius, - 0.5*Math.PI, red_line - 0.5*Math.PI);
  else
    {
      var start = zero_tick-0.5*Math.PI;//start drawing at 0;
      this.context.arc(0, 0, 0.9 * radius, start/*this.tick_start-0.5*Math.PI*/, red_line - 0.5*Math.PI, level<0);
    }
  this.context.stroke      ();
    
  //draw red line
  
  this.context.save        ();
  this.context.rotate      (red_line);
  this.context.beginPath   ();
  this.context.moveTo      (0, 0.45 * -radius);
  this.context.lineTo      (0, 0.8 * -radius);
  this.context.lineWidth   = radius * 0.05;
  this.context.strokeStyle = Speedometer_Widget.TOUCH_COLOR;
  this.context.stroke      ();
  this.context.restore     ();

  //second hand
  if(this.style == Speedometer_Widget.STYLE_TIMER)
  {
    red_line = this.scalef(60 * (level - Math.floor(level)), 0, 60, 0, Math.PI*2);
    this.context.save        ();
    this.context.rotate      (red_line);
    this.context.beginPath   ();
    this.context.moveTo      (0, 0.4 * -radius);
    this.context.lineTo      (0, 0.75 * -radius);
    this.context.lineWidth   = radius * 0.025;
    this.context.strokeStyle = Speedometer_Widget.TOUCH_COLOR;
    this.context.stroke      ();
    this.context.restore     ();    
  }

  if(this.goal_type != Speedometer_Widget.GOAL_TYPE_NONE)
    {
      this.context.lineWidth   = 0.05 * radius;
      this.context.strokeStyle = Speedometer_Widget.TOUCH_COLOR;
      this.context.fillStyle = this.context.strokeStyle;

  
      this.context.save        ();
      this.context.rotate      (this.goal_angle + 1.5*Math.PI);
      this.context.beginPath   ();
  
      //draw goal indicators
      //todo: surely this could be cleaner!
      if(this.goal_type == Speedometer_Widget.GOAL_TYPE_AT_LEAST)
        {
          if(this.style == Speedometer_Widget.STYLE_SPEEDOMETER)
            this.context.arc       (0, 0, 1.033*radius, 0, this.outline_angle - this.goal_angle);
          else if(this.style == Speedometer_Widget.STYLE_TIMER)
            this.context.arc       (0, 0, 1.033*radius, 0, 2*Math.PI - this.goal_angle, 0, true);
        }
      else if(this.goal_type == Speedometer_Widget.GOAL_TYPE_LESS_THAN)
        {
          if(this.style == Speedometer_Widget.STYLE_SPEEDOMETER)
            this.context.arc       (0, 0, 1.033*radius, -this.outline_angle - this.goal_angle, 0);
          else if(this.style == Speedometer_Widget.STYLE_TIMER)
            this.context.arc       (0, 0, 1.033*radius, 2*Math.PI - this.goal_angle, 0, false);
          //this.context.rotate    (-challenge_text.length * char_sapacing);
        }
      else if(this.goal_type == Speedometer_Widget.GOAL_TYPE_RANGE || this.goal_type == Speedometer_Widget.GOAL_TYPE_CONST_RANGE)
        {
          this.context.arc       (0, 0, 1.033*radius, 0, this.goal_angle_2 - this.goal_angle, this.goal_angle_2 < this.goal_angle);
          //this.context.rotate    ((0.5*(this.goal_angle_2 - this.goal_angle)) - (0.5* challenge_text.length * char_sapacing));
        }
  
      this.context.stroke      ();
  
      //draw the word "target"
      /*
      font_size = 0.1 * radius;
      this.context.font = "bold " + font_size + "px Muli Extrabold";
      var challenge_text       = "target";
      var char_sapacing        = 0.06;
      for(i=0; i<challenge_text.length; i++)
        {
          this.context.rotate    (char_sapacing);
          this.context.save();
          this.context.translate (1.14*radius, 0);
          this.context.rotate    (0.5*Math.PI);
          this.context.fillText(challenge_text.charAt(i), 0, 0); 
          this.context.restore();
        }
      */
  
      this.context.restore     ();
      
      //draw goal indicator
      this.context.shadowBlur = 10  ;
      this.context.shadowOffsetX = 5;
      this.context.shadowOffsetY = 5;
      
      this.context.fillStyle   = Speedometer_Widget.TOUCH_COLOR;
      this.context.save        ();
      this.context.rotate      (this.goal_angle);
      this.context.translate   (0, 0.817 * -radius);
      this.context.beginPath   ();
      this.context.arc         (0, 0, (1.09 * radius) - (0.85 * radius), 1.5*Math.PI - 0.45, 1.5*Math.PI + 0.45);
      this.context.lineTo      (0, 0);
      this.context.fill        ();
      this.context.restore     ();
  
  
      //draw second goal indicator. hey, mike, have you heard of functions?
      if((this.goal_type == Speedometer_Widget.GOAL_TYPE_RANGE) || (this.goal_type == Speedometer_Widget.GOAL_TYPE_CONST_RANGE))
        {
          this.context.save        ();
          this.context.rotate      (this.goal_angle_2);
          this.context.translate   (0, 0.817 * -radius);
          this.context.beginPath   ();
          this.context.arc         (0, 0, (1.09 * radius) - (0.85 * radius), 1.5*Math.PI - 0.45, 1.5*Math.PI + 0.45);
          this.context.lineTo      (0, 0);
          this.context.fill        ();
          this.context.restore     ();
        }
    } //end goal_type_none
  
  this.context.shadowBlur = 0  ;
  this.context.shadowOffsetX = 0;
  this.context.shadowOffsetY = 0;
  
  //draw text
  this.context.restore     ();
  font_size = 0.5*radius;
  this.context.font = "bold " + font_size + "px " + Speedometer_Widget.FONT + " Bold";
  
  this.context.fillStyle = Speedometer_Widget.PASS_COLOR;
  if(this.style == Speedometer_Widget.STYLE_TIMER)
    this.context.fillText(Math.floor(num), this.canvas.width*0.5, this.canvas.height*0.5);
  else
    this.context.fillText(num.toFixed(this.num_digits), this.canvas.width*0.5, this.canvas.height*0.5);
  
  
  this.context.fillStyle = Speedometer_Widget.MAIN_COLOR;
  this.context.font = 0.25 * font_size + "px " + Speedometer_Widget.FONT + " Bold";
  this.context.fillText(this.num_text, this.canvas.width*0.5, 0.47 * (this.canvas.height - font_size));
  
  
  this.context.font = 0.25 * font_size + "px " + Speedometer_Widget.FONT + " Bold";
  if(this.style == Speedometer_Widget.STYLE_TIMER)
    this.context.fillText(this.get_timer_string(num), this.canvas.width*0.5, 0.58 * (this.canvas.height + font_size));
  //else
    //this.context.fillText(level.toFixed(this.level_digits) + " " + this.level_text, this.canvas.width*0.5, 0.58 * (this.canvas.height + font_size));
    //this.context.fillText(this.level_text, this.canvas.width*0.5, 0.58 * (this.canvas.height + font_size));
};

//PRIVATE
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.onmousedown = function(e)
{ 
  this.mouse_is_down = true;
  var x = e.offsetX - this.canvas.offsetWidth * 0.5;
  var y = e.offsetY - this.canvas.offsetHeight * 0.5;
  var r = Math.sqrt(x*x + y*y);
  var big_radius   = 0.5 * Math.min(this.canvas.offsetWidth, this.canvas.offsetHeight);
    
  if(r > big_radius * 0.6)
    {
      //var t = Math.atan2(y, x) + Math.PI * 0.5;
      var t = Math.atan2(y, x) + Math.PI * 0.5;
      if(this.style == Speedometer_Widget.STYLE_TIMER)
        {if(t<0) t += 2*Math.PI;}
      else
        {if(t > Math.PI) t -= Math.PI*2;}
      if((t > (this.goal_angle - 0.35)) &&  (t < (this.goal_angle + 0.35)))
        this.mouse_is_down_in_goal_indicator = true;
      else if(((this.goal_type == Speedometer_Widget.GOAL_TYPE_RANGE) || (this.goal_type == Speedometer_Widget.GOAL_TYPE_CONST_RANGE)) && (t > (this.goal_angle_2 - 0.35)) &&  (t < (this.goal_angle_2 + 0.35)))
        this.mouse_is_down_in_goal_indicator_2 = true;
    }
};

//PRIVATE
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.onmouseup   = function(e)
{
  this.mouse_is_down_in_goal_indicator   = false;
  this.mouse_is_down_in_goal_indicator_2 = false;
};

//PRIVATE
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.onmousemove = function(e)
{
  if(this.mouse_is_down_in_goal_indicator || this.mouse_is_down_in_goal_indicator_2)
    {
      var big_radius   = 0.5 * Math.min(this.canvas.offsetWidth, this.canvas.offsetHeight);
      var x = e.offsetX - this.canvas.offsetWidth * 0.5;
      var y = e.offsetY - this.canvas.offsetHeight * 0.5;
      var r = Math.sqrt(x*x + y*y);
      if(r > 0)
        {
          var t = Math.atan2(y, x) + Math.PI * 0.5;
          if(this.style == Speedometer_Widget.STYLE_TIMER)
            {if(t<0) t += 2*Math.PI;}
          else
            {if(t > Math.PI) t -= Math.PI*2;}
          var g  = this.scalef(t , this.tick_start, this.tick_end, this.level_min, this.level_max);
          
          if(this.goal_type == Speedometer_Widget.GOAL_TYPE_CONST_RANGE)
            {
              if(this.mouse_is_down_in_goal_indicator)
                this.set_goal_2(g + this.range, false);
              else
                this.set_goal(g - this.range, false);
            }
          
          if(this.mouse_is_down_in_goal_indicator)
            this.set_goal(g, false);
          else
            this.set_goal_2(g, false);
        }
  }
};

/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.touchstart = function(e)
{
  var mousePos = this.get_touch_position(this.canvas, e);
  var touch = e.touches[0];
  var mouseEvent = new MouseEvent("mousedown", 
    {
      clientX: touch.clientX,
      clientY: touch.clientY
    });
  this.canvas.dispatchEvent(mouseEvent);
};

/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.touchend = function(e)
{
  var mouseEvent = new MouseEvent("mouseup", {});
  this.canvas.dispatchEvent(mouseEvent);
};

/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.touchmove = function(e)
{
  var touch = e.touches[0];
  var mouseEvent = new MouseEvent("mousemove",
    {
      clientX: touch.clientX,
      clientY: touch.clientY
    });
  this.canvas.dispatchEvent(mouseEvent);
};

/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.prevent_touch_scroll = function(e)
{
  if ((e.target == this.canvas) && (this.mouse_is_down_in_goal_indicator)) 
    e.preventDefault();
  else if ((e.target == this.canvas) && (this.mouse_is_down_in_goal_indicator_2) && (this.style == Speedometer_Widget.STYLE_RANGE)) 
    e.preventDefault();
};

/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.get_touch_position = function(canvas, e) 
{
  var rect = canvas.getBoundingClientRect();
  var result =
    {
      x: e.touches[0].clientX - rect.left,
      y: e.touches[0].clientY - rect.top
    };
  return result;
};

//PUBLIC
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.get_timer_string = function(value)
{
  /* expects value to be minutes */
  value *= 60;
  var val = Math.round(value);
  var secs = val % 60;
  var mins = (val-secs) / 60;
  var time = mins.toFixed(0) + ":" + ("0" + secs).slice(-2);
  return time;
};

//PRIVATE
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.scalef = function(x, in_min, in_max, out_min, out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
};

//PRIVATE
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.clip = function(x, min, max)
{
  return (x < min) ? min : (x > max) ? max : x;
};

//PRIVATE
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.create_image = function(src)
{
  var img = document.createElement("IMG");
  img.src = src;
  img.style.display = "none";
  document.body.appendChild(img);
  return img;
};

//PRIVATE
/*--------------------------------------------------------------------*/
// for widgets of STYLE_RANGE
//returns 0 if in range, -1 if lower, +1 if higher
Speedometer_Widget.prototype.value_is_in_goal_range = function()
{
  if(this.goal_type == Speedometer_Widget.GOAL_TYPE_LESS_THAN)
    {
      return (this.target_level <= this.current_goal) ? 0 : 1;
    }
  else if(this.goal_type == Speedometer_Widget.GOAL_TYPE_AT_LEAST)
    {
      return (this.target_level >= this.current_goal) ? 0 : -1;
    }
  else if((this.goal_type == Speedometer_Widget.GOAL_TYPE_RANGE) || (this.goal_type == Speedometer_Widget.GOAL_TYPE_CONST_RANGE))
    {
      var larger = this.current_goal > this.current_goal_2;
      var max = (larger) ? this.current_goal : this.current_goal_2;
      var min = (larger) ? this.current_goal_2 : this.current_goal;
      var result = 0;
      if(this.target_level < min)
        result = -1;
      else if (this.target_level > max)
        result = 1;
      return result;
    }
  else if(this.goal_type == Speedometer_Widget.GOAL_TYPE_NONE)
    {
      return 0;
    }
};

//PRIVATE
/*--------------------------------------------------------------------*/
Speedometer_Widget.prototype.interpolation_thread_run_loop_run = function()
{
  this.current_level  = (0.8 * this.current_level ) + (0.2 * this.target_level  );
  this.current_num    = (0.8 * this.current_num   ) + (0.2 * this.target_num    );
  this.current_goal   = (0.5 * this.current_goal  ) + (0.5 * this.target_goal   );
  this.current_goal_2 = (0.5 * this.current_goal_2) + (0.5 * this.target_goal_2 );
  this.draw(this.current_level, this.current_num, this.current_goal, this.current_goal_2);
  this.update_thread = setTimeout(this.interpolation_thread_run_loop_run.bind(this), this.interpolation_thread_runloop_duration);
};



