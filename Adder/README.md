# Adder

In this folder you can find the adder.xsa file and the project-spec folder that contains all the different driver versions (the path of the driver files is `project-spec/meta-user/recipes_modules/adderdriver`).
If you want to try this on your machine the steps are:

1. Create a new petalinux project (look in `Tutorials/04_petalinux_workflow.md` for the creation and also the following steps) **external** to this repository
2. Add the `axi4_cpu_interface_wrapper.xsa` file to your local project (once again refer to the `Tutorials` folder)
3. Copy the `project-spec` folder from the repo to your local project
4. Run `petalinux-build`. In theory it should work, if not I'm gonna cry and we'll probably find a better way of sharing projects