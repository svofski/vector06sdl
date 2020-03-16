package com.svofski.v06x;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLU;
import android.util.Log;

import com.svofski.v06x.util.RawResourceReader;
import com.svofski.v06x.util.ShaderHelper;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import static javax.microedition.khronos.opengles.GL10.GL_TEXTURE_2D;

public class EmulatorRenderer implements GLSurfaceView.Renderer {
    private Quad quad = new Quad();
    private Context mContext;
    private int mWidth = 0;
    private int mHeight = 0;
    private ByteBuffer mTextureBuffer = null;
    private int mTextureBufferSize = 0;
    private int mTexture = -1;
    private int mProgramHandle = 0;

    EmulatorRenderer(Context context) {
        mContext = context;
    }

    private int allocTexture(GL10 gl)
    {
        IntBuffer textures = IntBuffer.allocate(2);
        gl.glGenTextures(1, textures);
        return textures.get(0);
    }

    private void freeTexture(GL10 gl, int texture)
    {
        IntBuffer textures = IntBuffer.allocate(2);
        textures.put(texture);
        gl.glDeleteTextures(1, textures);
    }

    private void updateTexture(GL10 gl)
    {
        final byte[] pixels = MainActivity.emulator().getPixels();
        final int width = MainActivity.emulator().getPixelsWidth();
        final int height = MainActivity.emulator().getPixelsHeight();

        if (mTextureBuffer == null || mTextureBufferSize != pixels.length) {
            mTextureBuffer = ByteBuffer.allocate(pixels.length);
            mTextureBufferSize = pixels.length;
        }
        mTextureBuffer.put(pixels);
        mTextureBuffer.position(0);

        boolean new_texture = mTexture == -1;
        if (mTexture == -1) {
            mTexture = allocTexture(gl);
        }
        gl.glActiveTexture(GL10.GL_TEXTURE0);
        gl.glEnable(GL10.GL_TEXTURE_2D);
        gl.glBindTexture(GL10.GL_TEXTURE_2D, mTexture);
        gl.glTexEnvx(GL10.GL_TEXTURE_ENV, GL10.GL_TEXTURE_ENV_MODE, GL10.GL_REPLACE);
        gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);

        //gl.glPixelStorei(GL10.GL_UNPACK_ALIGNMENT, 1);
        //gl.glPixelStorei(GL10.GL_PACK_ALIGNMENT, 1);

        if (new_texture) {
            gl.glTexImage2D(GL10.GL_TEXTURE_2D, 0, GL10.GL_RGBA, width, height, 0, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, mTextureBuffer);
        }
        else {
            gl.glTexSubImage2D(GL10.GL_TEXTURE_2D, 0, 0, 0, width, height, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, mTextureBuffer);
        }
        gl.glBindTexture(GL10.GL_TEXTURE_2D, 0);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        gl.glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        gl.glDisable(GL10.GL_CULL_FACE);
        gl.glDisable(GL10.GL_DEPTH_TEST);
        gl.glDisable(GL10.GL_ALPHA_TEST);
        gl.glShadeModel(GL10.GL_SMOOTH);
        gl.glDisable(GL10.GL_DITHER);
        gl.glEnable(GL10.GL_TEXTURE_2D);
        //gl.glEnable(GL10.GL_BLEND);
        //setBlendMode(BLEND_MODE_NORMAL);

        //createShaders(gl);
    }

    void createShaders(GL10 gl) {
        final String vertexShader = getVertexShader();
        final String fragmentShader = getFragmentShader();

        final int vertexShaderHandle = ShaderHelper.compileShader(GLES20.GL_VERTEX_SHADER, vertexShader);
        final int fragmentShaderHandle = ShaderHelper.compileShader(GLES20.GL_FRAGMENT_SHADER, fragmentShader);

        mProgramHandle = ShaderHelper.createAndLinkProgram(vertexShaderHandle, fragmentShaderHandle,
                new String[] {"a_Position",  "a_Color"});
    }

    private String getVertexShader() {
        return RawResourceReader.readTextFileFromRawResource(mContext, R.raw.singlepass_vsh);
    }

    private String getFragmentShader() {
        return RawResourceReader.readTextFileFromRawResource(mContext, R.raw.singlepass_fsh);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        // Sets the current view port to the new size.
        gl.glViewport(0, 0, width, height);
        // Select the projection matrix
        gl.glMatrixMode(GL10.GL_PROJECTION);
        // Reset the projection matrix
        gl.glLoadIdentity();

        // Calculate the aspect ratio of the window
        //GLU.gluPerspective(gl, 45.0f, (float) width / (float) height, 0.1f, 100.0f);

        gl.glOrthof (0, width, height, 0, -1f, 1f);
        gl.glMatrixMode (GL10.GL_MODELVIEW);
        gl.glLoadIdentity ();

        mWidth = width;
        mHeight = height;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        gl.glClear(GL10.GL_COLOR_BUFFER_BIT);
        gl.glLoadIdentity();
        gl.glScalef(mWidth,mHeight,1);
        //gl.glTranslatef(0f,0f,-10f);
        updateTexture(gl);
        quad.draw(gl, mTexture);
        gl.glLoadIdentity();

        if (++mFrameCount >= 100) {
            long now = System.currentTimeMillis();
            float fps = 1000f * mFrameCount / (now - mTime);
            mTime = now;
            mFrameCount = 0;
            Log.d("v06x", "onDrawFrame FPS: " + fps);
        }
    }

    private int mFrameCount = 0;
    private long mTime = System.currentTimeMillis();
}

class Quad
{
    private FloatBuffer mVertexBuffer;
    private FloatBuffer mColorBuffer;
    private FloatBuffer mTextureCoordsBuffer;
    private ByteBuffer mIndexBuffer;

    private static final float[] QUAD_COORDS = new float[]
            {
                    1f,1f,0f,
                    0f,1f,0f,
                    1f,0f,0f,
                    0f,0f,0f,
            };

    private static final float[] TEXTURE_COORDS = new float[]
            {
                    1.0f, 1.0f,
                    0.0f, 1.0f,
                    1.0f, 0.0f,
                    0.0f, 0.0f,
            };

    private float COLORS[] = {
            1.0f,  0.0f,  0.0f,  1.0f,
            0.0f,  1.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.0f,  1.0f,  1.0f,
    };

    private byte indices[] = {
            3, 0, 1, 3, 0, 2
    };


    public Quad() {
        ByteBuffer byteBuf = ByteBuffer.allocateDirect(QUAD_COORDS.length * 4);
        byteBuf.order(ByteOrder.nativeOrder());
        mVertexBuffer = byteBuf.asFloatBuffer();
        mVertexBuffer.put(QUAD_COORDS);
        mVertexBuffer.position(0);

        byteBuf = ByteBuffer.allocateDirect(COLORS.length * 4);
        byteBuf.order(ByteOrder.nativeOrder());
        mColorBuffer = byteBuf.asFloatBuffer();
        mColorBuffer.put(COLORS);
        mColorBuffer.position(0);

        byteBuf = ByteBuffer.allocateDirect(TEXTURE_COORDS.length * 4);
        byteBuf.order(ByteOrder.nativeOrder());
        mTextureCoordsBuffer = byteBuf.asFloatBuffer();
        mTextureCoordsBuffer.put(TEXTURE_COORDS);
        mTextureCoordsBuffer.position(0);

        mIndexBuffer = ByteBuffer.allocateDirect(indices.length);
        mIndexBuffer.put(indices);
        mIndexBuffer.position(0);
    }

    public void draw(GL10 gl, int texture) {
        gl.glActiveTexture(GL10.GL_TEXTURE0);
        gl.glBindTexture(GL10.GL_TEXTURE_2D, texture);

        gl.glVertexPointer(3, GL10.GL_FLOAT, 0, mVertexBuffer);
        gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mTextureCoordsBuffer);
        gl.glColorPointer(4, GL10.GL_FLOAT, 0, mColorBuffer);

        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glEnableClientState(GL10.GL_COLOR_ARRAY);
        gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);

        gl.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        //gl.glEnable(GL10.GL_BLEND);
        //gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);

        gl.glDrawElements(GL10.GL_TRIANGLES, indices.length, GL10.GL_UNSIGNED_BYTE,
                mIndexBuffer);

        gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glDisableClientState(GL10.GL_COLOR_ARRAY);
        gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
    }
}