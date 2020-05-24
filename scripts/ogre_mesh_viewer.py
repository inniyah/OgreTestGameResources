#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import Ogre
import Ogre.RTShader as OgreRTShader
import Ogre.Bites as OgreBites
import Ogre.Overlay as OgreOverlay

import os.path

RGN_MESHVIEWER = "OgreMeshViewer"

VES2STR = ("ERROR", "Position", "Blend Weights", "Blend Indices", "Normal", "Diffuse", "Specular", "Texcoord", "Binormal", "Tangent")
VET2STR = ("float", "float2", "float3", "float4", "ERROR", "short", "short2", "short3", "short4", "ubyte4", "argb", "abgr")

def show_vertex_decl(decl):
    OgreOverlay.Columns(2)
    OgreOverlay.Text("Semantic")
    OgreOverlay.NextColumn()
    OgreOverlay.Text("Type")
    OgreOverlay.NextColumn()
    OgreOverlay.Separator()

    for e in decl.getElements():
        OgreOverlay.Text(VES2STR[e.getSemantic()])
        OgreOverlay.NextColumn()
        try:
            OgreOverlay.Text(VET2STR[e.getType()])
        except IndexError:
            OgreOverlay.Text("TODO")
        OgreOverlay.NextColumn()

    OgreOverlay.Columns(1)

class MaterialCreator(Ogre.MeshSerializerListener):
    def __init__(self):
        Ogre.MeshSerializerListener.__init__(self)

    def processMaterialName(self, mesh, name):
        # ensure some material exists so we can display the name
        mat_mgr = Ogre.MaterialManager.getSingleton()
        if not mat_mgr.resourceExists(name, mesh.getGroup()):
            try:
                mat_mgr.create(name, mesh.getGroup())
            except RuntimeError:
                # do not crash if name is ""
                # this is illegal due to OGRE specs, but we want to show that in the UI
                pass

    def processSkeletonName(self, mesh, name):
        pass

    def processMeshCompleted(self, mesh):
        pass

class MeshViewer(OgreBites.ApplicationContext, OgreBites.InputListener):

    def __init__(self, meshname, rescfg):
        OgreBites.ApplicationContext.__init__(self, "OgreMeshViewer")
        OgreBites.InputListener.__init__(self)

        self.show_about = False
        self.show_metrics = False
        self.meshname = os.path.basename(meshname)
        self.meshdir = os.path.dirname(meshname)
        self.rescfg = rescfg
        self.highlighted = -1
        self.orig_mat = None
        self.highlight_mat = None
        self.restart = False

        self.active_controllers = {}

    def keyPressed(self, evt):
        if evt.keysym.sym == OgreBites.SDLK_ESCAPE:
            self.getRoot().queueEndRendering()
        elif evt.keysym.sym == ord("b"):
            self._toggle_bbox()
        elif evt.keysym.sym == ord("a"):
            self._toggle_axes()
        elif evt.keysym.sym == ord("p"):
            self._save_screenshot()

        return True

    def mousePressed(self, evt):
        if evt.clicks != 2:
            return True

        vp = self.cam.getViewport()
        ray = self.cam.getCameraToViewportRay(evt.x / vp.getActualWidth(), evt.y / vp.getActualHeight())
        self.ray_query.setRay(ray)
        for hit in self.ray_query.execute():
            self.camman.setPivotOffset(ray.getPoint(hit.distance))
            break

        return True

    def _toggle_bbox(self):
        enode = self.entity.getParentSceneNode()
        enode.showBoundingBox(not enode.getShowBoundingBox())

    def _toggle_axes(self):
        self.axes.setVisible(not self.axes.getVisible())

    def _save_screenshot(self):
        name = os.path.splitext(self.meshname)[0]
        outpath = os.path.join(self.meshdir, "screenshot_{}_".format(name))

        self.cam.getViewport().setOverlaysEnabled(False)
        self.getRoot().renderOneFrame()
        self.getRenderWindow().writeContentsToTimestampedFile(outpath, ".png")
        self.cam.getViewport().setOverlaysEnabled(True)

    def draw_about(self):
        flags = OgreOverlay.ImGuiWindowFlags_AlwaysAutoResize
        self.show_about = OgreOverlay.Begin("About OgreMeshViewer", self.show_about, flags)[1]
        OgreOverlay.Text("By Pavel Rojtberg")
        OgreOverlay.Text("OgreMeshViewer is licensed under the MIT License, see LICENSE for more information.")
        OgreOverlay.Separator()
        OgreOverlay.BulletText("Ogre:  %s" % Ogre.__version__)
        OgreOverlay.BulletText("imgui: %s" % OgreOverlay.GetVersion())
        OgreOverlay.End()

    def draw_metrics(self):
        win = self.getRenderWindow()
        stats = win.getStatistics()

        OgreOverlay.SetNextWindowPos(OgreOverlay.ImVec2(win.getWidth() - 10, win.getHeight() - 10),
                                     OgreOverlay.ImGuiCond_Always, OgreOverlay.ImVec2(1, 1))
        OgreOverlay.SetNextWindowBgAlpha(0.3)
        flags = OgreOverlay.ImGuiWindowFlags_NoMove \
              | OgreOverlay.ImGuiWindowFlags_NoTitleBar \
              | OgreOverlay.ImGuiWindowFlags_NoResize \
              | OgreOverlay.ImGuiWindowFlags_AlwaysAutoResize \
              | OgreOverlay.ImGuiWindowFlags_NoSavedSettings \
              | OgreOverlay.ImGuiWindowFlags_NoFocusOnAppearing \
              | OgreOverlay.ImGuiWindowFlags_NoNav
        self.show_metrics = OgreOverlay.Begin("Metrics", self.show_metrics, flags)[1]
        OgreOverlay.Text("Metrics")
        OgreOverlay.Separator()
        OgreOverlay.Text("Average FPS: {:.2f}".format(stats.avgFPS))
        OgreOverlay.Text("Batches: {}".format(stats.batchCount))
        OgreOverlay.Text("Triangles: {}".format(stats.triangleCount))
        OgreOverlay.End()

    def frameStarted(self, evt):
        OgreBites.ApplicationContext.frameStarted(self, evt)

        if not self.cam.getViewport().getOverlaysEnabled():
            return True

        OgreOverlay.ImGuiOverlay.NewFrame(evt)

        if OgreOverlay.BeginMainMenuBar():
            if OgreOverlay.BeginMenu("File"):
                if OgreOverlay.MenuItem("Select Renderer"):
                    self.getRoot().queueEndRendering()
                    self.restart = True
                if OgreOverlay.MenuItem("Save Screenshot", "P"):
                    self._save_screenshot()
                if OgreOverlay.MenuItem("Quit", "Esc"):
                    self.getRoot().queueEndRendering()
                OgreOverlay.EndMenu()
            if OgreOverlay.BeginMenu("View"):
                enode = self.entity.getParentSceneNode()
                if OgreOverlay.MenuItem("Show Axes", "A", self.axes.getVisible()):
                    self._toggle_axes()
                if OgreOverlay.MenuItem("Show Bounding Box", "B", enode.getShowBoundingBox()):
                    self._toggle_bbox()
                if self.entity.hasSkeleton() and OgreOverlay.MenuItem("Show Skeleton", None, self.entity.getDisplaySkeleton()):
                    self.entity.setDisplaySkeleton(not self.entity.getDisplaySkeleton())
                OgreOverlay.EndMenu()

            if OgreOverlay.BeginMenu("Help"):
                if OgreOverlay.MenuItem("Metrics", None, self.show_metrics):
                    self.show_metrics = not self.show_metrics
                if OgreOverlay.MenuItem("About"):
                    self.show_about = True
                OgreOverlay.EndMenu()

            OgreOverlay.EndMainMenuBar()

        if self.show_about:
            self.draw_about()

        if self.show_metrics:
            self.draw_metrics()

        # Mesh Info Sidebar
        mesh = Ogre.MeshManager.getSingleton().getByName(self.meshname)

        OgreOverlay.SetNextWindowPos(OgreOverlay.ImVec2(0, 30))
        flags = OgreOverlay.ImGuiWindowFlags_NoTitleBar \
              | OgreOverlay.ImGuiWindowFlags_NoMove
        OgreOverlay.Begin("MeshProps", None, flags)
        OgreOverlay.Text(self.meshname)

        highlight = -1

        if OgreOverlay.CollapsingHeader("Geometry"):
            if mesh.sharedVertexData:
                if OgreOverlay.TreeNode("Shared Vertices: {}".format(mesh.sharedVertexData.vertexCount)):
                    show_vertex_decl(mesh.sharedVertexData.vertexDeclaration)
                    TreePop()
            else:
                OgreOverlay.Text("Shared Vertices: None")

            for i, sm in enumerate(mesh.getSubMeshes()):
                submesh_details = OgreOverlay.TreeNode("SubMesh #{}".format(i))
                if OgreOverlay.IsItemHovered():
                    highlight = i

                if submesh_details:
                    OgreOverlay.BulletText("Material: {}".format(sm.getMaterialName()))
                    if sm.indexData:
                        bits = sm.indexData.indexBuffer.getIndexSize() * 8
                        OgreOverlay.BulletText("Indices: {} ({} bit)".format(sm.indexData.indexCount, bits))
                    else:
                        OgreOverlay.BulletText("Indices: None")

                    if sm.vertexData:
                        if OgreOverlay.TreeNode("Vertices: {}".format(sm.vertexData.vertexCount)):
                            show_vertex_decl(sm.vertexData.vertexDeclaration)
                            OgreOverlay.TreePop()
                    else:
                        OgreOverlay.BulletText("Vertices: shared")
                    OgreOverlay.TreePop()

        if self.highlighted > -1:
            self.entity.getSubEntities()[self.highlighted].setMaterialName(self.orig_mat)

        if highlight > -1:
            self.orig_mat = self.entity.getSubEntities()[highlight].getMaterial().getName()
            self.entity.getSubEntities()[highlight].setMaterial(self.highlight_mat)
            self.highlighted = highlight

        animations = self.entity.getAllAnimationStates()
        if animations is not None and OgreOverlay.CollapsingHeader("Animations"):
            controller_mgr = Ogre.ControllerManager.getSingleton()

            if self.entity.hasSkeleton():
                OgreOverlay.Text("Skeleton: {}".format(mesh.getSkeletonName()))
                # self.entity.setUpdateBoundingBoxFromSkeleton(True)
            if mesh.hasVertexAnimation():
                OgreOverlay.Text("Vertex Animations")

            for name, astate in animations.getAnimationStates().items():
                if OgreOverlay.TreeNode(name):
                    if astate.getEnabled():
                        if OgreOverlay.Button("Reset"):
                            astate.setEnabled(False)
                            astate.setTimePosition(0)
                            if name in self.active_controllers:
                                controller_mgr.destroyController(self.active_controllers[name])
                    elif OgreOverlay.Button("Play"):
                        astate.setEnabled(True)
                        self.active_controllers[name] = controller_mgr.createFrameTimePassthroughController(
                            Ogre.AnimationStateControllerValue.create(astate, True))
                    changed = False
                    if astate.getLength() > 0:
                        OgreOverlay.SameLine()
                        changed, value = OgreOverlay.SliderFloat("", astate.getTimePosition(), 0, astate.getLength(), "%.3fs")
                    if changed:
                        astate.setEnabled(True)
                        astate.setTimePosition(value)
                    OgreOverlay.TreePop()

        if OgreOverlay.CollapsingHeader("Bounds"):
            bounds = mesh.getBounds()
            s = bounds.getSize()
            OgreOverlay.BulletText("Size: {:.2f}, {:.2f}, {:.2f}".format(s[0], s[1], s[2]))
            c = bounds.getCenter()
            OgreOverlay.BulletText("Center: {:.2f}, {:.2f}, {:.2f}".format(c[0], c[1], c[2]))
            OgreOverlay.BulletText("Radius: {:.2f}".format(mesh.getBoundingSphereRadius()))

        OgreOverlay.End()

        # ShowDemoWindow()

        return True

    def locateResources(self):
        rgm = Ogre.ResourceGroupManager.getSingleton()
        # ensure our resource group is separate, even with a local resources.cfg
        rgm.createResourceGroup(RGN_MESHVIEWER, False)

        # use parent implementation to locate system-wide RTShaderLib
        OgreBites.ApplicationContext.locateResources(self)

        # allow override by local resources.cfg
        if not self.getFSLayer().fileExists("resources.cfg"):
            # we use the fonts from SdkTrays.zip
            trays_loc = self.getDefaultMediaDir()+"/packs/SdkTrays.zip"
            rgm.addResourceLocation(trays_loc, "Zip", RGN_MESHVIEWER)

        if self.rescfg:
            cfg = Ogre.ConfigFile()
            cfg.loadDirect(self.rescfg)

            for sec, settings in cfg.getSettingsBySection().items():
                for kind, loc in settings.items():
                    rgm.addResourceLocation(loc, kind, sec)

        # explicitly add mesh location to be safe
        if not rgm.resourceLocationExists(self.meshdir, Ogre.RGN_DEFAULT):
            rgm.addResourceLocation(self.meshdir, "FileSystem", Ogre.RGN_DEFAULT)

    def setup(self):
        OgreBites.ApplicationContext.setup(self)
        self.addInputListener(self)

        self.restart = False
        imgui_overlay = OgreOverlay.ImGuiOverlay()
        OgreOverlay.GetIO().IniFilename = self.getFSLayer().getWritablePath("imgui.ini")

        root = self.getRoot()
        scn_mgr = root.createSceneManager()
        scn_mgr.addRenderQueueListener(self.getOverlaySystem())
        self.scn_mgr = scn_mgr

        # set listener to deal with missing materials
        self.mat_creator = MaterialCreator()
        Ogre.MeshManager.getSingleton().setListener(self.mat_creator)

        # for picking
        self.ray_query = scn_mgr.createRayQuery(Ogre.Ray())

        #imgui_overlay.addFont("SdkTrays/Value", RGN_MESHVIEWER)
        imgui_overlay.show()
        OgreOverlay.OverlayManager.getSingleton().addOverlay(imgui_overlay)
        imgui_overlay.disown() # owned by OverlayMgr now

        shadergen = OgreRTShader.ShaderGenerator.getSingleton()
        shadergen.addSceneManager(scn_mgr)  # must be done before we do anything with the scene

        scn_mgr.setAmbientLight(Ogre.ColourValue(.1, .1, .1))

        self.highlight_mat = Ogre.MaterialManager.getSingleton().create("Highlight", RGN_MESHVIEWER)
        self.highlight_mat.getTechniques()[0].getPasses()[0].setEmissive(Ogre.ColourValue(1, 1, 0))

        self.entity = scn_mgr.createEntity(self.meshname)
        scn_mgr.getRootSceneNode().createChildSceneNode().attachObject(self.entity)

        diam = self.entity.getBoundingBox().getSize().length()

        axes_node = scn_mgr.getRootSceneNode().createChildSceneNode()
        axes_node.getDebugRenderable()  # make sure Ogre/Debug/AxesMesh is created
        self.axes = scn_mgr.createEntity("Ogre/Debug/AxesMesh")
        axes_node.attachObject(self.axes)
        axes_node.setScale(Ogre.Vector3(diam / 4))
        self.axes.setVisible(False)
        self.axes.setQueryFlags(0) # exclude from picking

        self.cam = scn_mgr.createCamera("myCam")
        self.cam.setNearClipDistance(diam * 0.01)
        self.cam.setAutoAspectRatio(True)
        camnode = scn_mgr.getRootSceneNode().createChildSceneNode()
        camnode.attachObject(self.cam)

        light = scn_mgr.createLight("MainLight")
        light.setType(Ogre.Light.LT_DIRECTIONAL)
        light.setSpecularColour(Ogre.ColourValue.White)
        camnode.attachObject(light)

        vp = self.getRenderWindow().addViewport(self.cam)
        vp.setBackgroundColour(Ogre.ColourValue(.3, .3, .3))

        self.camman = OgreBites.CameraMan(camnode)
        self.camman.setStyle(OgreBites.CS_ORBIT)
        self.camman.setYawPitchDist(Ogre.Radian(0), Ogre.Radian(0.3), diam)
        self.camman.setFixedYaw(False)

        self.imgui_input = OgreBites.ImGuiInputListener()
        self.input_dispatcher = OgreBites.InputListenerChain([self.imgui_input, self.camman])
        self.addInputListener(self.input_dispatcher)
    
    def shutdown(self):
        OgreBites.ApplicationContext.shutdown(self)
        
        if self.restart:
            # make sure empty rendersystem is written
            self.getRoot().shutdown()
            self.getRoot().setRenderSystem(None)

if __name__ == "__main__":
    import argparse

    default_resources = os.path.join(os.path.dirname(os.path.abspath(__file__)), "resources.cfg")
    if not os.path.exists(default_resources):
        default_resources = None

    parser = argparse.ArgumentParser(description="Ogre Mesh Viewer")
    parser.add_argument("meshfile", help="path to a .mesh")
    parser.add_argument("-c", "--rescfg", help="path to the resources.cfg", default=default_resources)
    args = parser.parse_args() 
    app = MeshViewer(args.meshfile, args.rescfg)

    while True: # allow auto restart
        app.initApp()
        app.getRoot().startRendering()
        app.closeApp()

        if not app.restart: break
