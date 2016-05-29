VAGRANTFILE_API_VERSION = "2"
ENV['VAGRANT_DEFAULT_PROVIDER'] = 'virtualbox'

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|

  config.vm.box = "ubuntu/trusty64"
  config.vm.synced_folder "./test-scripts", "/home/vagrant/test-scripts"
  config.vm.synced_folder "./test-output", "/home/vagrant/test-output"

  config.vm.provider "virtualbox" do |v|
    v.customize ["modifyvm", :id, "--memory", "4096"]
  end

  config.vm.provision :file, source: 'patch.diff', destination: '~/patch.diff'

  config.vm.provision "shell" do |s|
    s.path = "./provision.sh"
  end

end
