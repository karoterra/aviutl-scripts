export class GLSL {
  static vsSrc = `attribute vec3 position;
void main(void){
    gl_Position = vec4(position, 1.0);
}`;

  static vsSrcES300 = `#version 300 es
in vec3 position;
void main(void){
  gl_Position = vec4(position, 1.0);
}`;

  static position = new Float32Array([
    -1.0, 1.0, 0.0,
    1.0, 1.0, 0.0,
    -1.0, -1.0, 0.0,
    1.0, -1.0, 0.0
  ]);
  static index = new Int16Array([
    0, 2, 1,
    1, 2, 3
  ]);

  constructor(canvas) {
    this.canvas = canvas;
    this.uniLocation = new Array();
    this.gl = canvas.getContext('webgl2');
    this.vs = null;
    this.fs = null;
    this.program = null;

    this.file = 'a';
  }

  createShader(src, type) {
    const shader = this.gl.createShader(type);
    this.gl.shaderSource(shader, src);
    this.gl.compileShader(shader);
    if (this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)) {
      return shader;
    } else {
      console.error(this.gl.getShaderInfoLog(shader));
      return null;
    }
  }

  createProgram() {
    const program = this.gl.createProgram();
    this.gl.attachShader(program, this.vs);
    this.gl.attachShader(program, this.fs);
    this.gl.linkProgram(program);
    if (!this.gl.getProgramParameter(program, this.gl.LINK_STATUS)){
      console.error(this.gl.getProgramInfoLog(program));
      return null;
    }
    this.gl.useProgram(program);

    this.uniLocation[0] = this.gl.getUniformLocation(program, 'time');
    this.uniLocation[1] = this.gl.getUniformLocation(program, 'totalTime');
    this.uniLocation[2] = this.gl.getUniformLocation(program, 'frame');
    this.uniLocation[3] = this.gl.getUniformLocation(program, 'totalFrame');
    this.uniLocation[4] = this.gl.getUniformLocation(program, 'resolution');
    this.uniLocation[5] = this.gl.getUniformLocation(program, 'track');

    const vbo = this.gl.createBuffer();
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, vbo);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, GLSL.position, this.gl.STATIC_DRAW);
    const attLocation = this.gl.getAttribLocation(program, 'position');
    this.gl.enableVertexAttribArray(attLocation);
    this.gl.vertexAttribPointer(attLocation, 3, this.gl.FLOAT, false, 0, 0);

    const ibo = this.gl.createBuffer();
    this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, ibo);
    this.gl.bufferData(this.gl.ELEMENT_ARRAY_BUFFER, GLSL.index, this.gl.STATIC_DRAW);

    this.gl.clearColor(0.0, 0.0, 0.0, 0.0);

    return program;
  }

  async loadFragmentShader(url) {
    if (this.program) {
      this.gl.deleteProgram(this.program);
      this.program = null;
    }

    const src = await fetch(url).then(r => r.text());
    this.fs = this.createShader(src, this.gl.FRAGMENT_SHADER);
    if (src.startsWith('#version 300 es')) {
      this.vs = this.createShader(GLSL.vsSrcES300, this.gl.VERTEX_SHADER);
    } else {
      this.vs = this.createShader(GLSL.vsSrc, this.gl.VERTEX_SHADER);
    }

    if (this.fs == null) return;
    this.program = this.createProgram();
  }

  resize() {
    const viewport = this.gl.getParameter(this.gl.VIEWPORT);
    if (viewport[2] !== window.innerWidth || viewport[3] !== window.innerHeight) {
      this.canvas.width = window.innerWidth;
      this.canvas.height = window.innerHeight;
      this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);
    }
  }

  clear() {
    this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);
  }

  render(params) {
    this.resize();

    this.clear();

    this.gl.uniform1f(this.uniLocation[0], params.time);
    this.gl.uniform1f(this.uniLocation[1], params.totaltime);
    this.gl.uniform1i(this.uniLocation[2], params.frame);
    this.gl.uniform1i(this.uniLocation[3], params.totalframe);
    this.gl.uniform2fv(this.uniLocation[4], [this.canvas.width, this.canvas.height]);
    this.gl.uniform4fv(this.uniLocation[5], params.track);

    this.gl.drawElements(this.gl.TRIANGLES, 6, this.gl.UNSIGNED_SHORT, 0);
    this.gl.flush();
  }
}
